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

#include "DfxParser.h"
#include <fstream>

namespace dfx
{
	std::string to_string(DfxResult result)
	{
		std::string s;

		switch (result)
		{
			case DfxResult::NoError: s = "NoError"; break;
			case DfxResult::FileNotFound: s = "File not found"; break;
			case DfxResult::FileOpenError: "Error opening file"; break;
			case DfxResult::InvalidFileType: s = "Invalid file type"; break;
			case DfxResult::ParsingError: s = "Parsing error"; break;
			case DfxResult::MustBeSpecified: s = "Must be specified"; break;
			case DfxResult::MustBeString: s = "Must be a double quoted string"; break;
			case DfxResult::NoteMissing: s = "Drum note missing"; break;
			case DfxResult::NoteMustBeWholeNumber: s = "Note must be whole number"; break;
			case DfxResult::KitsMissing: s = "Kits are missing"; break;
			case DfxResult::KitValWrongType: s = "Kit value must be a {}-list type"; break;
			case DfxResult::InstrumentIncludeDataMissing: s = "Instrument include file data is missing."; break;
			case DfxResult::InstrumentsMissing: s = "Instruments are missing"; break;
			case DfxResult::InstrumentsMustBeList: s = "Instruments must be in a {}-list"; break;
			case DfxResult::DrumValMustBeList: s = "Drum info must be in a {}-list"; break;
			case DfxResult::VelocitiesMissing: s = "Velocity layers are missing"; break;
			case DfxResult::VelocitiesMustBeNonEmptySquareList: s = "Velocity layers must be in a non-empty array"; break;
			case DfxResult::InvalidVelocityCode: s = "Invalid velocity code"; break;
			case DfxResult::RobinsMissing: s = "Robins are missing"; break;
			case DfxResult::RobinsMustBeNonEmptySquareList: s = "Robins must be in a non-empty []-list"; break;
			case DfxResult::RobinMustBeNameValue: s = "Robin must be a name-value pair"; break;
			case DfxResult::RobinNameMustBeValidPath: s = "Robin name must be valid path"; break;
			case DfxResult::OffsetMustBeWholeNumber: s = "Offset must be whole number"; break;
			case DfxResult::OffsetMissing: s = "offset must be specified"; break;
			case DfxResult::PeakMustBeNumber: s = "Peak must be whole or floating point number (suffix units allowed)"; break;
			case DfxResult::PeakMissing: s = "peak must be specified"; break;
			case DfxResult::RmsMustBeNumber: s = "Rms must be whole or floating point number (suffix units allowed)"; break;
			case DfxResult::RmsMissing: s = "rms must be specified"; break;
			case DfxResult::ValueHasWrongUnits: s = "value can only have ratio units"; break;
			case DfxResult::ValueNotLegal: s = "value when converted to unitless number must be in range 0 < val <= 1.0"; break;
			case DfxResult::VerifyFailed: s = "Verify failed"; break;
			case DfxResult::UnspecifiedError: s = "Unspecified error"; break;
			default: break;
		}

		return s;
	}

	// ///////////////////////////////////////////////////////////////
	ParserResult DfxParser::LoadFile(std::ostream &serr, std::string_view &fname)
	{
		auto result = Parser::LoadFile(fname);

		if (result == ParserResult::NoError)
		{
			sound_font_path = fname;
			sound_font_path = sound_font_path.generic_string();
		}
		else
		{
			serr << "Parsing error encountered:\n";
			PrintError(serr, fname);
		}

		return result;
	}

	// ///////////////////////////////////////////////////////////////
	DfxResult DfxParser::LoadAndVerify(std::ostream& serr, std::string_view& fname, bool as_include)
	{
		StartLog(serr);

		auto load_result = LoadFile(serr, fname);

		if (load_result == ParserResult::NoError)
		{
			auto result = DfxResult::NoError;
			bool bx;

			if (as_include)
			{
				bx = VerifyIncludeFile();
			}
			else bx = Verify();

			if (!bx)
			{
				result = DfxResult::VerifyFailed;
			}

			return result;
		}
		else
		{
			// @@ TODO: Investigate converting string_view into string.
			// It would seem using string_view is turning out to be a
			// real pain. I'm not sure it's worth it.
			
			// UPDATE: That's not the issue. The issue is that slog is null.

			//std::string zebra(fname.data());
			//return LogError(zebra, DfxResult::ParsingError);
			return LogError("opening file", DfxResult::ParsingError);
		}
	}


	// //////////////////////////////////////////////////////////////////
	// Verify

	// The outermost level of a drum font must be a name-value pair.
	// In bryx syntax, that is Dfx = {...}. In json syntax,
	// that is "Dfx" : { ... }. Note that the BryParser has already
	// consumed the name part of this name-value pair. So we start with 
	// a value, and that value must be a {}-list. If it passes that test,
	// then we look deeper. Oh wait, it has already passed that test, 
	// in the parser itself. Still, we retrieve the pointer that points
	// to the {}-list and make sure it's not null for some reason.

	bool DfxParser::Verify()
	{
		errcnt = 0; // @@ TODO: Do this here, or in StartLog()? 

		const auto kits_map_ptr = GetKitsMapPtr();

		if (kits_map_ptr)
		{

			// Okay, the kits {}-list is a mapped (ie searchable) list of 
			// name-value pairs. If any entry in this list is not a name-value
			// pair, we log the error. Else, we dive down into the value part
			// of the pair. Oh wait! The parse has already guaranteed that each
			// member of the {}-list is a name-value pair, so that part should
			// always succeed. We we don't know is if the value part of each
			// pair has the right kind of data in it. So we call verify kit
			// to find out.
			// @@ TODO: What about duplicate names? Test this!

			for (auto nvkit : *kits_map_ptr)
			{
				VerifyKit(file_moniker, nvkit);
			}
		}
		else
		{
			LogError(file_moniker, DfxResult::KitsMissing);
		}

		return errcnt == 0;
	}

	// //////////////////////////////////////////////////////////////////
	// VerifyIncludeFile

	// The outermost level of a Dfx instrument include file must be a
	// name-value pair. In bryx syntax, that is Dfxi = {...}. In json
	// syntax, that is "Dfxi" : { ... }. Note that the BryParser has
	// already consumed the name part of this name-value pair. So we start
	// with  a value, and that value must be a {}-list. If it passes that
	// test, then we look deeper. Oh wait, it has already passed that test, 
	// in the parser itself. Still, we retrieve the pointer that points
	// to the {}-list and make sure it's not null for some reason.

	bool DfxParser::VerifyIncludeFile()
	{
		errcnt = 0; // @@ TODO: Do this here, or in StartLog()? 

		const auto curly_list_map_ptr = GetInstrumentIncludeMapPtr();

		if (curly_list_map_ptr)
		{
			// Okay, the {}-list should be mapped (ie searchable) list of 
			// one or two name-value pairs.
			// One of these name-value pairs is optional: a relative path
			// to the directory containing the variations (velocities and
			//robins) of the instrument. 
			// The other name-value pair should be named "velocities" with the
			// body value being a []-list of velocity layer specifications.
			// If the requirements above are not met, we log any error ecountered.

			// First, we verify that there is a path, it has the right form.
			static constexpr bool must_be_specified = false;
			VerifyPath(file_moniker, curly_list_map_ptr, must_be_specified);

			// Second, we verify the velocity layers, which must be there.
			VerifyVelocityLayers(file_moniker, curly_list_map_ptr);
		}
		else
		{
			LogError(file_moniker, DfxResult::InstrumentIncludeDataMissing);
		}

		return errcnt == 0;
	}

	bool DfxParser::VerifyKit(const std::string ctx, const nv_type& kit)
	{
		auto& kit_name = kit.first;
		auto& kit_val = kit.second;

		int save_errcnt = errcnt;

		// The value part of a kit should be another {}-list. If it's not,
		// we are out of here!

		// A dynamic type cast to a {}-list (aka "object map").

		auto kitmap_ptr = AsCurlyList(kit_val); 

		if (kitmap_ptr)
		{
			// Now the kit {}-list contains mainly a nested {}-list of instruments.
			// It might, however hold other properties of the kit, the main one
			// being an optional "relative path" to the instrument of the kit.
			// If we find such a path property, we make sure the value of that 
			// path is a valid path string. For all other properties that aren't
			// instruments, we'll skip over them for now, until I know what to do
			// with them.

			bool must_be_specified = false;
			VerifyPath(kit_name, kitmap_ptr, must_be_specified);

			// It also might have an optional include file base path

			must_be_specified = false;
			VerifyIncludeBasePath(kit_name, kitmap_ptr, must_be_specified);

			// Okay, on to the main show: the {}-list of instruments.

			auto vp = GetPropertyValue(kitmap_ptr, "instruments");

			if (vp)
			{
				// Well, there is an instrument property value. Is it the right type?
				// Ie. a {}-list?

				auto instrument_map_ptr = AsCurlyList(vp);

				if (instrument_map_ptr)
				{
					// Now walk that {}-list!
					VerifyInstruments(kit_name, instrument_map_ptr);
				}
				else
				{
					LogError(kit_name, DfxResult::InstrumentsMustBeList);
				}
			}
			else
			{
				LogError(kit_name, DfxResult::InstrumentsMissing);
			}
		}
		else
		{
			LogError(kit_name, DfxResult::KitValWrongType);
		}

		return errcnt == save_errcnt;
	}

	bool DfxParser::VerifyPath(const std::string ctx, const curly_list_type* parent_map, bool must_be_specified)
	{
		int save_errcnt = errcnt;

		// Check for a possibly optional "relative path".
		// If we find such a path property, we make sure the value of that 
		// path is a valid path string.

		auto new_ctx = ctx + '/' + "path";

		auto vp = GetPropertyValue(parent_map, "path");

		if (vp)
		{
			// Well, there is a path property. Is it the right type? 
			// Ie. a simple string value?

			auto svp = AsSimpleValue(vp);

			if (svp)
			{
				// Okay, it's a simple value. Is it a set of quoted characters?
				// Or unquoted characters?

				if (svp->tkn->type == TokenEnum::QuotedChars || svp->tkn->type == TokenEnum::UnquotedChars)
				{
					// @@ TODO: Does it look like a path?
				}
				else
				{
					LogError(new_ctx, DfxResult::MustBeString);
				}
			}
			else
			{
				LogError(new_ctx, DfxResult::MustBeString);
			}
		}
		else
		{
			if (must_be_specified)
			{
				LogError(new_ctx, DfxResult::MustBeSpecified);
			}
		}

		return errcnt == save_errcnt;
	}

	bool DfxParser::VerifyIncludeBasePath(const std::string ctx, const curly_list_type* parent_map, bool must_be_specified)
	{
		int save_errcnt = errcnt;

		// Check for a possibly optional "relative path".
		// If we find such a path property, we make sure the value of that 
		// path is a valid path string.

		auto new_ctx = ctx + '/' + "include_base_path";

		auto vp = GetPropertyValue(parent_map, "include_base_path");

		if (vp)
		{
			// Well, there is a path property. Is it the right type? 
			// Ie. a simple string value?

			auto svp = AsSimpleValue(vp);

			if (svp)
			{
				// Okay, it's a simple value. Is it a set of quoted characters?
				// Or unquoted characters?

				if (svp->tkn->type == TokenEnum::QuotedChars || svp->tkn->type == TokenEnum::UnquotedChars)
				{
					// @@ TODO: Does it look like a path?
					// @@ TODO: Can have $ as first argument, for macro like strings.
				}
				else
				{
					LogError(new_ctx, DfxResult::MustBeString);
				}
			}
			else
			{
				LogError(new_ctx, DfxResult::MustBeString);
			}
		}
		else
		{
			if (must_be_specified)
			{
				LogError(new_ctx, DfxResult::MustBeSpecified);
			}
		}

		return errcnt == save_errcnt;
	}

	bool DfxParser::VerifyInstruments(const std::string ctx, const curly_list_type* instrument_map_ptr)
	{
		int save_errcnt = errcnt;

		//auto ninstruments = instrument_map_ptr->size();

		// We have a {}-list of instruments. Walk that list!

		for (auto& drum_nv : *instrument_map_ptr)
		{
			VerifyInstrument(ctx, drum_nv);
		}

		return save_errcnt == errcnt;
	}

	bool DfxParser::VerifyInstrument(const std::string ctx, const nv_type& drum_nv)
	{
		auto& drum_name = drum_nv.first;
		auto& drum_val = drum_nv.second;

		auto new_ctx = ctx + '/' + drum_name;

		int save_errcnt = errcnt;

		// The value part of a kit should be another {}-list. If it's not,
		// we are out of here!

		// A dynamic type cast to a {}-list

		auto drum_map_ptr = AsCurlyList(drum_val);

		if (drum_map_ptr)
		{
			// Okay, an instrument spec must have at least a note mapping
			// (that is, what midi note does it map to.)

			bool must_be_specified = true;
			VerifyNote(new_ctx, drum_map_ptr, must_be_specified);

			// It can have an optional relative path and at least one
			// velocity layer. These can be inserted immediately in this
			// file, or they can be included from an external file.

			// See if the drum velocity layers are in an include file instead
			// of being immediate.

			auto ip = GetPropertyValue(drum_map_ptr, "include");

			if (ip)
			{
				// We defer full verify of the included text to
				// when the drum kit is built. Here, we simply
				// verify that our property looks like a file name.
				// Why do we defer it? Mainly because we don't have the 
				// full paths built yet and we don't want to parse
				// twice and/or don't want to store parse state of
				// the included file.
				VerifyFname(new_ctx, ip);
			}
			else
			{
				// The velocity layer stuff are given immediately.
				// It can have an optional relative path.

				must_be_specified = false;
				VerifyPath(new_ctx, drum_map_ptr, must_be_specified);

				// It must have at least one velocity layer.

				VerifyVelocityLayers(new_ctx, drum_map_ptr);
			}
		}
		else
		{
			LogError(new_ctx, DfxResult::DrumValMustBeList);
		}

		return save_errcnt == errcnt;
	}

	bool DfxParser::VerifyNote(const std::string ctx, const curly_list_type* parent_map, bool must_be_specified)
	{
		int save_errcnt = errcnt;

		// Check for a possibly optional "note".
		// If we find such a note property, we make sure the value of that 
		// path is a whole number.

		auto vp = GetPropertyValue(parent_map, "note");

		if (vp)
		{
			// Well, there is a note property. Is it the right type?
			// Ie. a whole number?

			auto svp = AsSimpleValue(vp);

			if (svp)
			{
				// Okay, it's a simple value. Is it a whole number?

				if (svp->tkn->IsWholeNumber())
				{
				}
				else
				{
					LogError(ctx, DfxResult::NoteMustBeWholeNumber);
				}
			}
			else
			{
				LogError(ctx, DfxResult::NoteMustBeWholeNumber);
			}
		}
		else
		{
			if (must_be_specified)
			{
				LogError(ctx, DfxResult::NoteMissing);
			}
		}

		return errcnt == save_errcnt;
	}

	bool DfxParser::VerifyVelocityLayers(const std::string ctx, const curly_list_type* parent_map)
	{
		int save_errcnt = errcnt;

		// See if the velocity layers property is present

		auto vlp = GetPropertyValue(parent_map, "velocities");

		if (vlp)
		{
			// The value must be an array, (vector of values), 
			// so we return a pointer to the vector below if
			// valid, else a nullptr

			auto velocities_ptr = AsSquareList(vlp);

			if (velocities_ptr)
			{
				if (velocities_ptr->size() > 0)
				{
					// Walk through the array (vector)
					for (auto vlayer_sh_ptr : *velocities_ptr)
					{
						VerifyVelocityLayer(ctx, vlayer_sh_ptr);
					}
				}
				else
				{
					LogError(ctx, DfxResult::VelocitiesMustBeNonEmptySquareList);
				}
			}
			else
			{
				LogError(ctx, DfxResult::VelocitiesMustBeNonEmptySquareList);
			}

		}
		else
		{
			LogError(ctx, DfxResult::VelocitiesMissing);
		}

		return errcnt == save_errcnt;
	}

	bool DfxParser::VerifyVelocityLayer(const std::string ctx, std::shared_ptr<Value>& vlayer_sh_ptr)
	{
		int save_errcnt = errcnt;

		auto corby = AsNameValue(vlayer_sh_ptr);

		if (corby)
		{
			auto& vel_code_str = corby->pair.first;
			//auto& vlayer_body = corby->pair.second;

			// vel_code must start with a "v"

			if (vel_code_str[0] == 'v')
			{
				// The rest of the characters must be digits between 0-9

				auto p = vel_code_str.c_str();
				++p; // past the v

				while (*p)
				{
					if (!Lexi::IsDigit(*p))
					{
						LogError(ctx, DfxResult::InvalidVelocityCode);
						break;
					}
					++p;
				}
			}
			else
			{
				LogError(ctx, DfxResult::InvalidVelocityCode);
			}

			// Okay, regardless of whether we have a valid velocity code,
			// let's check the body to see if it's kosher.

			// First off, it must be a {}-list

			auto& vlayer_body = corby->pair.second;

			auto vlayer_body_map_ptr =  AsCurlyList(vlayer_body);

			if (vlayer_body_map_ptr)
			{
				auto new_ctx = ctx + '/' + vel_code_str;

				// Might have an optional path
				bool must_be_specified = false;
				VerifyPath(new_ctx, vlayer_body_map_ptr, must_be_specified);

				// MUST have a non-empty robins array
				VerifyRobins(new_ctx, vlayer_body_map_ptr);
			}
			else
			{
				// We shouldn't get here, seein's how we already checked for
				// name value earlier, so vlayer_body *will* be value
				LogError(ctx, DfxResult::VelocityMustBeNameValue);
			}
		}
		else
		{
			LogError(ctx, DfxResult::VelocityMustBeNameValue);
		}

		return errcnt == save_errcnt;
	}


	bool DfxParser::VerifyRobins(const std::string ctx, const curly_list_type* parent_map_ptr)
	{
		int save_errcnt = errcnt;

		auto robins_arr_ptr = GetSquareListProperty(parent_map_ptr, "robins");

		if (robins_arr_ptr)
		{
			auto nrobins = robins_arr_ptr->size();
			if (nrobins > 0)
			{
				// Walk thru the robins. Each robin is represented as a
				// name-value pair.

				for (auto robin_sh_ptr : *robins_arr_ptr)
				{
					auto robin_nv_ptr  = AsNameValue(robin_sh_ptr);

					if (robin_nv_ptr)
					{
						VerifyRobin(ctx, robin_nv_ptr);
					}
					else
					{
						LogError(ctx, DfxResult::RobinMustBeNameValue);
					}
				}
			}
			else
			{
				LogError(ctx, DfxResult::RobinsMustBeNonEmptySquareList);
			}
		}
		else
		{
			LogError(ctx, DfxResult::RobinsMustBeNonEmptySquareList);
		}

		return errcnt == save_errcnt;
	}

	bool DfxParser::VerifyRobin(const std::string ctx, std::shared_ptr<NameValue> &robin_nv_ptr)
	{
		int save_errcnt = errcnt;

		// The name of a robin is arbitrary, but it must start with an "r".
		// @@ TODO: why? it doesn't really matter what it's called.

		auto& robin_name = robin_nv_ptr->pair.first;


		// Then:

		auto new_ctx = ctx + '/' + robin_name;

		// The body of a robin must be a {}-list having the file name to
		// the robin, plus three optional items: offset, peak, and rms.

		auto& robin_body = robin_nv_ptr->pair.second;

		auto robin_body_map_ptr = AsCurlyList(robin_body);

		if (robin_body_map_ptr)
		{
			bool must_be_specified = true;
			VerifyFname(ctx, robin_body_map_ptr, must_be_specified);

			must_be_specified = false;
			VerifyOffset(new_ctx, robin_body_map_ptr, must_be_specified);

			VerifyPeak(new_ctx, robin_body_map_ptr, must_be_specified);
			VerifyRMS(new_ctx, robin_body_map_ptr, must_be_specified);

			//auto peak_opt = GetSimpleProperty(robin_body_map_ptr, "peak");
			//auto rms_opt = GetSimpleProperty(robin_body_map_ptr, "rms");
		}
		else
		{
			// Should never reach here, cause we alerady verified earlier
			// it's a name value
			LogError(ctx, DfxResult::RobinMustBeNameValue);
		}

		return errcnt == save_errcnt;
	}

	bool DfxParser::VerifyFname(const std::string ctx, const curly_list_type* parent_map, bool must_be_specified)
	{
		int save_errcnt = errcnt;

		// Check for a possibly optional filename.
		// If we find such a property, we make sure the value 
		// is a valid fname.

		auto new_ctx = ctx + '/' + "fname";

		auto vp = GetPropertyValue(parent_map, "fname");

		if (vp)
		{
			VerifyFname(new_ctx, vp);
		}
		else
		{
			if (must_be_specified)
			{
				LogError(new_ctx, DfxResult::MustBeSpecified);
			}
		}

		return errcnt == save_errcnt;
	}


	bool DfxParser::VerifyFname(const std::string ctx, std::shared_ptr<Value> &vp)
	{
		int save_errcnt = errcnt;

		// Well, there is an fname property. Is it the right type? 
		// Ie. a simple string value?

		auto svp = AsSimpleValue(vp);

		if (svp)
		{
			// Okay, it's a simple value. Is it a string?

			if (svp->is_string())
			{
				// @@ TODO: Does it look like a fname?
			}
			else
			{
				LogError(ctx, DfxResult::MustBeString);
			}
		}
		else
		{
			LogError(ctx, DfxResult::MustBeString);
		}

		return errcnt == save_errcnt;
	}

	bool DfxParser::VerifyOffset(const std::string ctx, const curly_list_type* parent_map, bool must_be_specified)
	{
		int save_errcnt = errcnt;

		// Check for a possibly optional offset.
		// If we find such a property, we make sure the value it's a whole number.

		auto vp = GetPropertyValue(parent_map, "offset");

#if 1
		if (vp)
		{
			// We found the property. Is it the right type?
			// Is it a whole number?

			auto svp = AsSimpleValue(vp);

			if (svp)
			{
				// Okay, it's a simple value. Is it a whole number?

				if (svp->tkn->IsWholeNumber())
				{
				}
				else
				{
					LogError(ctx, DfxResult::OffsetMustBeWholeNumber);
				}
			}
			else
			{
				LogError(ctx, DfxResult::OffsetMustBeWholeNumber);
			}
		}
		else
		{
			if (must_be_specified)
			{
				LogError(ctx, DfxResult::OffsetMissing);
			}
		}

#endif

		return errcnt == save_errcnt;
	}

	bool DfxParser::VerifyPeak(const std::string ctx, const curly_list_type* parent_map, bool must_be_specified)
	{
		int save_errcnt = errcnt;

		auto new_ctx = ctx + '/' + "peak";

		// Check for a possibly optional peak specification.
		// If we find such a property, we make sure the value it's a number, 
		// whole or floating point doesn't matter. It may have an optional 
		// db unit on it.

		auto vp = GetPropertyValue(parent_map, "peak");

		if (vp)
		{
			// We found the property. Is it a number with the right range?

			auto num_tkn_ptr = ProcessAsNumber(new_ctx, vp);

			if (num_tkn_ptr)
			{
				// We've got a number. Is it in the right range?
				VerifyWaveMagnitude(new_ctx, num_tkn_ptr);
			}
			else
			{
				LogError(ctx, DfxResult::RmsMustBeNumber);
			}
		}
		else
		{
			if (must_be_specified)
			{
				LogError(ctx, DfxResult::PeakMissing);
			}
		}

		return errcnt == save_errcnt;
	}

	bool DfxParser::VerifyRMS(const std::string ctx, const curly_list_type* parent_map, bool must_be_specified)
	{
		int save_errcnt = errcnt;

		// Check for a possibly optional rms specification.
		// If we find such a property, we make sure the value it's a number, 
		// whole or floating point doesn't matter. It may have an optional 
		// db unit on it.

		auto new_ctx = ctx + '/' + "rms";

		auto vp = GetPropertyValue(parent_map, "rms");

		if (vp)
		{
			// We found the property. Is it a number with the right range?

			auto num_tkn_ptr = ProcessAsNumber(new_ctx, vp);

			if (num_tkn_ptr)
			{
				// We've got a number. Is it in the right range?
				VerifyWaveMagnitude(new_ctx, num_tkn_ptr);
			}
			else
			{
				LogError(ctx, DfxResult::RmsMustBeNumber);
			}
		}
		else
		{
			if (must_be_specified)
			{
				LogError(ctx, DfxResult::RmsMissing);
			}
		}

		return errcnt == save_errcnt;
	}

	token_ptr DfxParser::ProcessAsNumber(const std::string ctx, std::shared_ptr<Value> &vp)
	{
		auto svp = AsSimpleValue(vp);

		if (svp)
		{
			if (svp->is_number())
			{
				// Okay, it's already a number.
			}
			else
			{
				// It might be a number in a quoted string. (This would happen in
				// Json sytnax, for sure, but also allowed in bryx syntax.)

				// NOTE: Allows bryx unquoted number with units here too. Is this a problem?
				// Only if there were spaces. Lexi would kick out things like -30 db (with
				// space in between) so we're mostly okay here.

				// Returns error token if wasn't already a number or couldn't convert to one.
				token_ptr t = svp->CompatibleWithNumber();

				if (std::dynamic_pointer_cast<NumberToken>(t))
				{
					// Okay, the text string is compatible with a number.
					// We really want to do some surgery to the parse
					// tree and represent this value as a number. 
					// At first, I wasn't sure how to pull this off.

					svp->tkn = t;                  // Maybe this is all I need!
					svp->type = ValueEnum::Number; // Except have to do this too.
				}
				else
				{
					// t is actually an error token with further info
					auto err_t = std::dynamic_pointer_cast<SimpleToken>(t);
					auto& err_pkg = err_t->result_pkg;
					LogError(ctx, DfxResult::RmsMustBeNumber, err_pkg);
					return nullptr;
				}
			}
		}
		else
		{
			LogError(ctx, DfxResult::RmsMustBeNumber);
			return nullptr;
		}

		// Guess we have a token pointer that represents a number token
		return svp->tkn; 
	}

	bool DfxParser::VerifyWaveMagnitude(const std::string ctx, const token_ptr& tkn)
	{
		// Here, we know it's a number. But is it a properly formed number
		// that would serve as a wave file's peak or rms value? In these
		// cases the value must be 0 < val <= 1.0.
		// To verify this, we must account for units like "x", "X", "%", 
		// and "db", "dB".

		// NOTE: When building kit for real, we anticipate using the EngrNum
		// class, which preserves the mantissa for round tripping.

		int save_errcnt = errcnt;

		auto froglegs = std::dynamic_pointer_cast<NumberToken>(tkn);
		auto& traits = froglegs->number_traits;
		auto& engr_num = froglegs->engr_num;

		if (traits.HasExponent() || traits.HasMetricPrefix())
		{
			LogError(ctx, DfxResult::ValueNotLegal);
		}
		else
		{
			if (IsCat<UnitCatEnum::Ratio>(engr_num.units))
			{
				// Okay, we have ratio units. There are some constraints
				// we put on the number, specifically, it must not be 
				// negative or greater than 1.0

				auto num = engr_num.X(); // First, get the value, apply any scale too

				if (num < 0.0 || num > 1.0)
				{
					LogError(ctx, DfxResult::ValueNotLegal);
				}
			}
			else if (engr_num.units == UnitEnum::None)
			{
				// No units given, so the number must not be negative
				// and must be <= 1.0;

				auto num = engr_num.RawX();

				if (num < 0.0 || num > 1.0)
				{
					LogError(ctx, DfxResult::ValueNotLegal);
				}
			}
			else
			{
				LogError(ctx, DfxResult::ValueHasWrongUnits);
			}
		}

		return save_errcnt == errcnt;
	}

	/////////////////////////////////////////////////////////////////////

	void DfxParser::WriteDfx(std::ostream& sout)
	{
		WriteDfx(sout, lexi.syntax_mode);
	}

	void DfxParser::WriteDfx(std::ostream& sout, SyntaxModeEnum synmode)
	{
		lexi.syntax_mode = synmode;

		if (lexi.syntax_mode == SyntaxModeEnum::Bryx)
		{
			sout << file_moniker << " = \n";
		}
		else
		{
			sout << "\"" << file_moniker << "\":\n";
		}

		PrintWalk(sout, *root, 0);
		sout << std::endl;
	}

	// //////////////////////////////////////////////////////
	//
	// //////////////////////////////////////////////////////

	void DfxParser::StartLog(std::ostream& slog_)
	{
		slog = &slog_;
		errcnt = 0;
	}

	ParserResult DfxParser::LogError(const std::string ctx, ParserResult err)
	{
		std::ostream& sl = *slog;
		sl << "Context " << ctx << ": ";
		sl << to_string(err) << std::endl;
		++errcnt;
		return err;
	}


	DfxResult DfxParser::LogError(const std::string ctx, DfxResult err)
	{
		std::ostream & sl = *slog;
		sl << "Context " << ctx << ": ";
		sl << to_string(err) << std::endl;
		++errcnt;
		return err;
	}

	DfxResult DfxParser::LogError(const std::string ctx, DfxResult err, LexiResultPkg &err_pkg)
	{
		std::ostream& sl = *slog;
		sl << "Context " << ctx << ": ";
		sl << to_string(err) << std::endl;
		sl << "Lexical err --> " << err_pkg.msg << " near (" << err_pkg.extent.ecol << ")" << std::endl;
		++errcnt;
		return err;
	}

	void DfxParser::EndLog()
	{

	}

}
