#include "DrumFont.h"

namespace bryx
{
	std::shared_ptr<DrumKit> DrumFont::BuildKit(const nv_type& kit)
	{
		auto& kit_name = kit.first;
		auto& kit_val = kit.second;

		auto kitmap_ptr = ToObjectMap(kit_val);

		auto kit_path_opt = GetSimpleProperty(kitmap_ptr, "path");

		auto dk = std::make_shared<DrumKit>(kit_name, *kit_path_opt);

		auto instrument_map_ptr = GetInstruments(kitmap_ptr);

		BuildInstruments(dk, instrument_map_ptr);

		return dk;
	}

	void DrumFont::BuildInstruments(std::shared_ptr<DrumKit>& kit, const object_map_type* instrument_map_ptr)
	{
		auto ninstruments = instrument_map_ptr->size();
		kit->drums.reserve(ninstruments);

		for (auto& drum_nv : *instrument_map_ptr)
		{
			BuildInstrument(kit->drums, drum_nv);
		}
	}

	void DrumFont::BuildInstrument(std::vector<MultiLayeredDrum>& drums, const nv_type& drum_nv)
	{
		auto& drum_name = drum_nv.first;
		auto& drum_val = drum_nv.second;

		const object_map_type* drum_map_ptr = ToObjectMap(drum_val);

		auto vp = PropertyExists(drum_map_ptr, "note");

		auto svp = ToSimpleValue(vp);

		auto nt = std::dynamic_pointer_cast<NumberToken>(svp->tkn);

		int midiNote = static_cast<int>(nt->engr_num.X());

		auto drum_path_opt = GetSimpleProperty(drum_map_ptr, "path");

		std::cout << "drum name " << drum_name << std::endl;
		std::cout << "  path " << *drum_path_opt << std::endl;
		std::cout << "  note " << midiNote << std::endl;

		//auto drum = std::make_shared<MultiLayeredDrum>(drum_name, drum_path_opt, midiNote);
		//auto drum = MultiLayeredDrum(drum_name, *drum_path_opt, midiNote);
		MultiLayeredDrum drum(drum_name, *drum_path_opt, midiNote);

		// We should have a []-list of velocity layers. Each layer is represented
		// in the Parser as a name-value pair.

		const array_type* vlayers = GetArrayProperty(drum_map_ptr, "velocities");

		int nlayers = vlayers->size();
		drum.velocityLayers.reserve(nlayers);

		for (auto vlayer_sh_ptr : *vlayers)
		{
			BuildVelocityLayer(drum.velocityLayers, vlayer_sh_ptr);
		}

		drums.push_back(drum);
	}


	void DrumFont::BuildVelocityLayer(std::vector<std::shared_ptr<VelocityLayer>>& vlayers, std::shared_ptr<Value>& vlayer_sh_ptr)
	{
		auto frog = dynamic_cast<NameValue*>(vlayer_sh_ptr.get());

		auto& vel_code_str = frog->pair.first;
		auto& vlayer_body = frog->pair.second;

		// vel_code starts with a "v", so skip past that

		int vel_code = std::stoi(vel_code_str.substr(1));

		auto vlayer_body_map_ptr = ToObjectMap(vlayer_body);

		auto vlayer_path_opt = GetSimpleProperty(vlayer_body_map_ptr, "path");

		std::cout << "  velocity layer " << vel_code_str << std::endl;
		std::cout << "    path " << *vlayer_path_opt << std::endl;

		auto vlayer = std::make_shared<VelocityLayer>(*vlayer_path_opt, vel_code);

		// We should have a []-list of robins. Each robin is represented
		// in the Parser as a name-value pair.

		auto robins_arr_ptr = GetArrayProperty(vlayer_body_map_ptr, "robins");

		auto nrobins = robins_arr_ptr->size();

		vlayer->robinMgr.robins.reserve(nrobins);

		for (auto robin_sh_ptr : *robins_arr_ptr)
		{
			auto robin_nv_ptr = dynamic_cast<NameValue*>(robin_sh_ptr.get());
			BuildRobin(vlayer->robinMgr.robins, robin_nv_ptr);
		}

		vlayers.push_back(vlayer);
	}

	void DrumFont::BuildRobin(std::vector<Robin>& robins, NameValue* robin_nv_ptr)
	{
		// new: first is just a robin name. Note used. auto& fname = robin_nv_ptr->pair.first;
		auto& robin_body = robin_nv_ptr->pair.second;

		auto robin_body_map_ptr = ToObjectMap(robin_body);

		auto fname_opt = GetSimpleProperty(robin_body_map_ptr, "fname");

		auto offset_vp = PropertyExists(robin_body_map_ptr, "offset");
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

		auto peak_vp = PropertyExists(robin_body_map_ptr, "peak");
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

		auto rms_vp = PropertyExists(robin_body_map_ptr, "rms");
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

		//auto offset_opt = GetSimpleProperty(robin_body_map_ptr, "offset");
		//auto peak_opt = GetSimpleProperty(robin_body_map_ptr, "peak");
		//auto rms_opt = GetSimpleProperty(robin_body_map_ptr, "rms");

		std::cout << "      robin fname " << *fname_opt << std::endl;
		std::cout << "        offset " << offset << std::endl;
		std::cout << "        peak " << peak << std::endl;
		std::cout << "        rms " << rms << std::endl;

		//size_t offset = std::stoi(*offset_opt); // @@ TODO: Typing issue

		//double peak = std::stod(*peak_opt); // @@ TODO: handle dB units
		//double rms = std::stod(*rms_opt); // @@ TODO: handle dB units

		Robin robin(*fname_opt, peak, rms, offset);

		robins.emplace_back(std::move(robin));
	}

} // end of namespace