#include "DrumFont.h"

/******************************************************************************\
 * DFX - "Drum font exchange format" - source code
 *
 * Copyright (c) 2020 by Bryan Flamig
 *
 * This software helps facilitate the real-time playing of multi-layered drum
 * samples, by implementing a language that specifies a master directory of the
 * the sample wave files for a drum kit: where the samples are, what they are
 * for, and a summary of their properties. The DFX format allows modifications
 * of sample levels to achieve a unified, pleasing mix of sounds. Mechanisms
 * such as velocity layers and round robins are supported for this purpose.
 *
 * This exchange format has a one to one mapping to the widely used Json syntax,
 * simplified to be easier to read and write. It is easy to translate DFX files
 * into Json files that can be parsed by any software supporting Json syntax.
 *
 ******************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
\******************************************************************************/

namespace dfx
{
	using namespace bryx;

	DrumFont::DrumFont()
	: drumKits{}
	{
	}

	DfxResult DrumFont::LoadFile(std::ostream& sout, std::string_view &fname)
	{
		auto rv = DfxResult::NoError;

		StartLog(sout);

		auto rvp = DfxParser::LoadFile(sout, fname);

		if (rvp == ParserResult::NoError)
		{
			auto zebra = Verify();
			if (!zebra)
			{
				rv = DfxResult::VerifyFailed;
			}
			else
			{
				BuildFont();
			}
		}
		else
		{
			sout << "Parsing drum font file failed:" << std::endl;
			if (rvp == ParserResult::FileOpenError)
			{
				sout << "failed to open file: " << fname << std::endl;
			}
			else if (rvp == ParserResult::CannotDetermineSyntaxMode)
			{
				sout << "cannot determine syntax mode: " << fname << std::endl;
			}
			else
			{
				PrintError(sout, fname);
			}

			rv = DfxResult::UnspecifiedError; // @@ TODO: Last error?
		}

		EndLog();

		if (rv == DfxResult::NoError)
		{
			// But we might have had problems building the font
			if (errcnt > 0)
			{
				sout << errcnt << " Errors encountered building the font" << std::endl;
				rv = DfxResult::UnspecifiedError;
			}
		}

		return rv;
	}

	void DrumFont::BuildFont()
	{
		auto base_path = sound_font_path;
		base_path.remove_filename();

		using kits_map = bryx::curly_list_type;

		const kits_map* kits = GetKitsMapPtr();

		for (auto nvkit : *kits)
		{
			auto kit_ptr = BuildKit(base_path, nvkit);
			kit_ptr->FinishPaths(sound_font_path);
			kit_ptr->BuildNoteMap();
			drumKits.push_back(kit_ptr);
			int i = 42;
		}
	}

	void DrumFont::DumpRobins(std::ostream& sout)
	{
		for (auto& kit : drumKits)
		{
			for (auto& drum : kit->drums)
			{
				for (auto& layer : drum->velocityLayers)
				{
					for (auto& robin : layer.robinMgr.robins)
					{
						sout << "[" << layer.vrange.iMinVel << " - " << layer.vrange.iMaxVel << "]  ";
						sout << robin.fullPath << '\n';
					}
				}
			}
		}
	}

	std::shared_ptr<DrumKit> DrumFont::BuildKit(std::filesystem::path base_path, const nv_type& kit)
	{
		auto& kit_name = kit.first;
		auto& kit_val = kit.second;

		auto kitmap_ptr = AsCurlyList(kit_val);

		auto include_base_path_opt = GetSimpleProperty(kitmap_ptr, "include_base_path");
		if (!include_base_path_opt)
		{
			include_base_path_opt = "";
		}

		auto kit_path_opt = GetSimpleProperty(kitmap_ptr, "path");
		if (!kit_path_opt)
		{
			kit_path_opt = "";
		}

		auto dk = std::make_shared<DrumKit>(kit_name, base_path, *include_base_path_opt, *kit_path_opt);

		auto instrument_map_ptr = GetInstrumentMapPtr(kitmap_ptr);

		BuildInstruments(dk, instrument_map_ptr);

		return dk;
	}

	void DrumFont::BuildInstruments(std::shared_ptr<DrumKit>& kit, const curly_list_type* instrument_map_ptr)
	{
		auto ninstruments = instrument_map_ptr->size();
		kit->drums.reserve(ninstruments);

		for (auto& drum_nv : *instrument_map_ptr)
		{
			BuildInstrument(kit->drums, kit->cumulativePath, drum_nv);
		}
	}

	void DrumFont::BuildInstrument(std::vector<drum_ptr>& drums, std::filesystem::path cumulativePath, const nv_type& drum_nv)
	{
		auto& drum_name = drum_nv.first;
		auto& drum_val = drum_nv.second;

		const curly_list_type* drum_map_ptr = AsCurlyList(drum_val);

		auto vp = GetPropertyValue(drum_map_ptr, "note");
		auto svp = AsSimpleValue(vp);

		auto nt = std::dynamic_pointer_cast<NumberToken>(svp->tkn);

		int midi_note = static_cast<int>(nt->engr_num.X());

		// See if the drum velocity layers are in an include file instead
		// of being immediate.

		auto ip = GetPropertyValue(drum_map_ptr, "include");

		if (ip)
		{
			// Velocity layer stuff is in an include file.
			// So first we get the path. Now it defaults to
			// being relative to our cumulative path so far, but
			// if the include file name starts with "$fontbase/"
			// then we make it relative to the sound font file.
			// Then we load and verify the included information
			// and build the rest of the instrument from that.

			auto valptr = AsSimpleValue(ip);
			auto rel_path = valptr->tkn->to_string();

			std::filesystem::path include_path;

			if (rel_path.find("$fontbase/") == 0)
			{
				include_path = sound_font_path;
				include_path.remove_filename();
				include_path /= rel_path.erase(0, 10);
			}
			else
			{
				include_path = cumulativePath;
				include_path /= rel_path;
			}

			auto pstring = include_path.generic_string();
			auto psview = std::string_view(pstring.c_str());

			auto dp = std::make_unique<DfxParser>();
			static constexpr bool as_include = true;
			auto rv = dp->LoadAndVerify(*slog, psview, as_include);

			if (rv == DfxResult::NoError)
			{
				auto dmp = dp->GetInstrumentIncludeMapPtr();
				auto drum = MakeInstrument(drum_name, cumulativePath, midi_note, dmp);
				drums.push_back(std::move(drum));
			}
			else
			{
				// add errcnt from included parsing to main count
				errcnt += dp->errcnt;
			}
		}
		else
		{
			// Velocity layer stuff is embedded in main file. So easy peasy.
			auto drum = MakeInstrument(drum_name, cumulativePath, midi_note, drum_map_ptr);
			drums.push_back(std::move(drum));
		}
	}

	drum_ptr DrumFont::MakeInstrument(const std::string &drum_name, std::filesystem::path cumulativePath, int midi_note, const curly_list_type *drum_map_ptr)
	{
		auto drum_path_opt = GetSimpleProperty(drum_map_ptr, "path");

		std::cout << "drum " << drum_name << std::endl;
		std::cout << "  path " << '"' << *drum_path_opt << '"' << std::endl;
		std::cout << "  note " << midi_note << std::endl;

		auto drum = std::make_shared<MultiLayeredDrum>(drum_name, cumulativePath, *drum_path_opt, midi_note);

		// We should have a []-list of velocity layers. Each layer is represented
		// in the Parser as a name-value pair.

		const square_list_type* vlayers = GetSquareListProperty(drum_map_ptr, "velocities");

		int nlayers = vlayers->size();
		drum->velocityLayers.reserve(nlayers);

		for (auto vlayer_sh_ptr : *vlayers)
		{
			BuildVelocityLayer(drum->velocityLayers, vlayer_sh_ptr);
		}

		return drum;
	}

	void DrumFont::BuildVelocityLayer(std::vector<VelocityLayer>& vlayers, std::shared_ptr<Value>& vlayer_sh_ptr)
	{
		auto nvp = dynamic_cast<NameValue*>(vlayer_sh_ptr.get());

		auto& vel_code_str = nvp->pair.first;
		auto& vlayer_body = nvp->pair.second;

		// vel_code starts with a "v", so skip past that

		int vel_code = std::stoi(vel_code_str.substr(1));

		auto vlayer_body_map_ptr = AsCurlyList(vlayer_body);

		auto vlayer_path_opt = GetSimpleProperty(vlayer_body_map_ptr, "path");

		std::cout << "  velocity layer " << vel_code_str << std::endl;
		std::cout << "    path " << '"' << *vlayer_path_opt << '"' << std::endl;

		VelocityLayer vlayer(*vlayer_path_opt, vel_code);

		// We should have a []-list of robins. Each robin is represented
		// in the Parser as a name-value pair.

		auto robins_arr_ptr = GetSquareListProperty(vlayer_body_map_ptr, "robins");

		auto nrobins = robins_arr_ptr->size();

		vlayer.robinMgr.robins.reserve(nrobins);

		for (auto robin_sh_ptr : *robins_arr_ptr)
		{
			auto robin_nv_ptr = dynamic_cast<NameValue*>(robin_sh_ptr.get());
			BuildRobin(vlayer.robinMgr.robins, robin_nv_ptr);
		}

		vlayers.emplace_back(std::move(vlayer));
	}

	void DrumFont::BuildRobin(std::vector<Robin>& robins, NameValue* robin_nv_ptr)
	{
		// new: first is just a robin name. Note used. auto& fname = robin_nv_ptr->pair.first;
		auto& robin_body = robin_nv_ptr->pair.second;

		auto robin_body_map_ptr = AsCurlyList(robin_body);

		auto fname_opt = GetSimpleProperty(robin_body_map_ptr, "fname");

		auto offset_vp = GetPropertyValue(robin_body_map_ptr, "offset");
		int offset;

		if (offset_vp)
		{
			auto t = ProcessAsNumber("BuildRobin", offset_vp);
			auto nt = std::dynamic_pointer_cast<NumberToken>(t);
			offset = static_cast<int>(nt->engr_num.X());
		}
		else
		{
			// Use default
			offset = 0;
		}

		auto peak_vp = GetPropertyValue(robin_body_map_ptr, "peak");
		double peak;

		if (peak_vp)
		{
			auto t = ProcessAsNumber("BuildRobin", peak_vp);
			auto nt = std::dynamic_pointer_cast<NumberToken>(t);
			peak = nt->engr_num.X();
		}
		else
		{
			// Use default
			peak = 1.0;
		}

		auto rms_vp = GetPropertyValue(robin_body_map_ptr, "rms");
		double rms;

		if (rms_vp)
		{
			auto t = ProcessAsNumber("BuildRobin", rms_vp);
			auto nt = std::dynamic_pointer_cast<NumberToken>(t);
			rms = nt->engr_num.X();
		}
		else
		{
			// Use default
			rms = 1.0;
		}

		std::cout << "      robin " << '"' << *fname_opt << '"' << std::endl;
		std::cout << "        offset " << offset << std::endl;
		std::cout << "        peak " << peak << std::endl;
		std::cout << "        rms " << rms << std::endl;

		Robin robin(*fname_opt, peak, rms, offset);

		robins.emplace_back(std::move(robin));
	}

} // end of namespace