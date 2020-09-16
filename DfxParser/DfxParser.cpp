#include "DfxParser.h"

namespace bryx
{

	std::string to_string(DfxVerifyResult result)
	{
		std::string s;

		switch (result)
		{
			case DfxVerifyResult::NoError: s = "NoError"; break;
			case DfxVerifyResult::FileNotFound: s = "File not found"; break;
			case DfxVerifyResult::InvalidFileType: s = "Invalid file type"; break;
			case DfxVerifyResult::PathMustBeSpecified: s = "Path must be specified"; break;
			case DfxVerifyResult::PathMustBeString: s = "Path must be a double quoted string"; break;
			case DfxVerifyResult::NoteMissing: s = "Drum note missing"; break;
			case DfxVerifyResult::NoteMustBeWholeNumber: s = "Note must be whole number"; break;
			case DfxVerifyResult::KitsMissing: s = "Kits are missing"; break;
			case DfxVerifyResult::KitValWrongType: s = "Kit value must be object type"; break;
			case DfxVerifyResult::InstrumentsMissing: s = "Instruments are missing"; break;
			case DfxVerifyResult::InstrumentsMustBeList: s = "Instruments must be in a {}-list"; break;
			case DfxVerifyResult::DrumValMustBeList: s = "Drum info must be in a {}-list"; break;
			case DfxVerifyResult::VelocitiesMissing: s = "Velocity layers are missing"; break;
			case DfxVerifyResult::VelocitiesMustBeNonEmptyArray: s = "Velocity layers must be in a non-empty array"; break;
			case DfxVerifyResult::InvalidVelocityCode: s = "Invalid velocity code"; break;
			case DfxVerifyResult::RobinsMissing: s = "Robins are missing"; break;
			case DfxVerifyResult::RobinsMustBeNonEmptyArray: s = "Robins must be in a non-empty []-list"; break;
			case DfxVerifyResult::RobinMustBeNameValue: s = "Robin must be a name-value pair"; break;
			case DfxVerifyResult::RobinNameMustBeValidPath: s = "Robin name must be valid path"; break;
			case DfxVerifyResult::OffsetMustBeWholeNumber: s = "Offset must be whole number"; break;
			case DfxVerifyResult::OffsetMissing: s = "offset must be specified"; break;
			case DfxVerifyResult::PeakMustBeNumber: s = "Peak must be whole or floating point number (suffix units allowed)"; break;
			case DfxVerifyResult::PeakMissing: s = "peak must be specified"; break;
			case DfxVerifyResult::RmsMustBeNumber: s = "Rms must be whole or floating point number (suffix units allowed)"; break;
			case DfxVerifyResult::RmsMissing: s = "rms must be specified"; break;
			case DfxVerifyResult::ValueHasWrongUnits: s = "value can only have ratio units"; break;
			case DfxVerifyResult::ValueNotLegal: s = "value when converted to unitless number must be in range 0 < val <= 1.0"; break;
			case DfxVerifyResult::UnspecifiedError: s = "Unspecified error"; break;
			default: break;
		}

		return s;
	}

	DfxVerifyResultPkg::DfxVerifyResultPkg()
	: msg()
	, code(DfxVerifyResult::NoError)
	{
	}

	DfxVerifyResultPkg::DfxVerifyResultPkg(std::string msg_, DfxVerifyResult code_)
	: DfxVerifyResultPkg()
	{
		msg = msg_;
		code = code_;
	}

	void DfxVerifyResultPkg::Clear()
	{
		code = DfxVerifyResult::NoError;
		ResetMsg();
	}

	void DfxVerifyResultPkg::ResetMsg()
	{
		msg.clear();
	}

	void DfxVerifyResultPkg::Print(std::ostream& sout) const
	{
		sout << to_string(code) << ": " << msg << std::endl;
	}

	// ///////////////////////////////////////////////////////////////
	ParserResult DfxParser::LoadFile(std::string fname)
	{
		auto result = ParserResult::NoError;

		std::fstream f(fname, std::ios_base::in);

		if (!f.is_open())
		{
			result = ParserResult::FileOpenError;
		}
		else
		{
			sound_font_path = fname;

			auto fbuf = f.rdbuf();
			SetStreamBuf(fbuf);
			result = Parse();
		}

		return result;
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

		const auto kits_map_ptr = GetKits();

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
			LogError(file_moniker, DfxVerifyResult::KitsMissing);
		}

		return errcnt == 0;
	}

	bool DfxParser::VerifyKit(const std::string zzz, const nv_type& kit)
	{
		auto& kit_name = kit.first;
		auto& kit_val = kit.second;

		int save_errcnt = errcnt;

		// The value part of a kit should be another {}-list. If it's not,
		// we are out of here!

		// A dynamic type cast to a {}-list (aka "object map").

		auto kitmap_ptr = ToObjectMap(kit_val); 

		if (kitmap_ptr)
		{
			// Now the kit {}-list contains mainly a nested {}-list of instruments.
			// It might, however hold other properties of the kit, the main one
			// being an optional "relative path" to the instrument of the kit.
			// If we find such a path property, we make sure the value of that 
			// path is a valid path string. For all other properties that aren't
			// instruments, we'll skip over them for now, until I know what to do
			// with them.

			bool path_must_be_specified = false;
			VerifyPath(kit_name, kitmap_ptr, path_must_be_specified);

			// Okay, on to the main show: the {}-list of instruments.

			auto vp = PropertyExists(kitmap_ptr, "instruments");

			if (vp)
			{
				// Well, there is an instrument property. Is it the right type?
				// Ie. a object map using a {}-list?

				auto instrument_map_ptr = ToObjectMap(vp);

				if (instrument_map_ptr)
				{
					// Now walk that {}-list!
					VerifyInstruments(kit_name, instrument_map_ptr);
				}
				else
				{
					LogError(kit_name, DfxVerifyResult::InstrumentsMustBeList);
				}
			}
			else
			{
				LogError(kit_name, DfxVerifyResult::InstrumentsMissing);
			}
		}
		else
		{
			LogError(kit_name, DfxVerifyResult::KitValWrongType);
		}

		return errcnt == save_errcnt;
	}

	bool DfxParser::VerifyPath(const std::string zzz, const object_map_type* parent_map, bool path_must_be_specified)
	{
		int save_errcnt = errcnt;

		// Check for a possibly optional "relative path".
		// If we find such a path property, we make sure the value of that 
		// path is a valid path string.

		auto new_zzz = zzz + '/' + "path";

		auto vp = PropertyExists(parent_map, "path");

		if (vp)
		{
			// Well, there is a path property. Is it the right type? 
			// Ie. a simple string value?

			auto svp = ToSimpleValue(vp);

			if (svp)
			{
				// Okay, it's a simple value. Is it a set of quoted characters?
				// Or unquoted characters?

				if (svp->tkn.type == TokenEnum::QuotedChars || svp->tkn.type == TokenEnum::UnquotedChars)
				{
					// @@ TODO: Does it look like a path?
				}
				else
				{
					LogError(new_zzz, DfxVerifyResult::PathMustBeString);
				}
			}
			else
			{
				LogError(new_zzz, DfxVerifyResult::PathMustBeString);
			}
		}
		else
		{
			if (path_must_be_specified)
			{
				LogError(new_zzz, DfxVerifyResult::PathMustBeSpecified);
			}
		}

		return errcnt == save_errcnt;
	}

	bool DfxParser::VerifyInstruments(const std::string zzz, const object_map_type* instrument_map_ptr)
	{
		int save_errcnt = errcnt;

		//auto ninstruments = instrument_map_ptr->size();

		// We have a {}-list of instruments. Walk that list!

		for (auto& drum_nv : *instrument_map_ptr)
		{
			VerifyInstrument(zzz, drum_nv);
		}

		return save_errcnt == errcnt;
	}

	bool DfxParser::VerifyInstrument(const std::string zzz, const nv_type& drum_nv)
	{
		auto& drum_name = drum_nv.first;
		auto& drum_val = drum_nv.second;

		auto new_zzz = zzz + '/' + drum_name;

		int save_errcnt = errcnt;

		// The value part of a kit should be another {}-list. If it's not,
		// we are out of here!

		// A dynamic type cast to a {}-list (aka "object map").

		auto drum_map_ptr = ToObjectMap(drum_val);

		if (drum_map_ptr)
		{
			// Okay, an instrument spec must have at least a note mapping
			// (that is, what midi note does it map to.)
			// It can have an optional relative path.
			// It must have at least one velocity layer.

			bool path_must_be_specified = false;
			VerifyPath(new_zzz, drum_map_ptr, path_must_be_specified);

			// Look for note

			bool note_must_be_specified = true;
			VerifyNote(new_zzz, drum_map_ptr, note_must_be_specified);

			// Look for at least one velocity layer

			VerifyVelocityLayers(new_zzz, drum_map_ptr);
		}
		else
		{
			LogError(new_zzz, DfxVerifyResult::DrumValMustBeList);
		}

		return save_errcnt == errcnt;
	}

	bool DfxParser::VerifyNote(const std::string zzz, const object_map_type* parent_map, bool note_must_be_specified)
	{
		int save_errcnt = errcnt;

		// Check for a possibly optional "note".
		// If we find such a note property, we make sure the value of that 
		// path is a whole number.

		auto vp = PropertyExists(parent_map, "note");

		if (vp)
		{
			// Well, there is a note property. Is it the right type?
			// Ie. a whole number?

			auto svp = ToSimpleValue(vp);

			if (svp)
			{
				// Okay, it's a simple value. Is it a whole number?

				if (svp->tkn.type == TokenEnum::WholeNumber)
				{
				}
				else
				{
					LogError(zzz, DfxVerifyResult::NoteMustBeWholeNumber);
				}
			}
			else
			{
				LogError(zzz, DfxVerifyResult::NoteMustBeWholeNumber);
			}
		}
		else
		{
			if (note_must_be_specified)
			{
				LogError(zzz, DfxVerifyResult::NoteMissing);
			}
		}

		return errcnt == save_errcnt;
	}

	bool DfxParser::VerifyVelocityLayers(const std::string zzz, const object_map_type* parent_map)
	{
		int save_errcnt = errcnt;

		// See if the velocity layers object is present

		auto vlp = PropertyExists(parent_map, "velocities");

		if (vlp)
		{
			// The value must be an array, (vector of values), 
			// so we return a pointer to the vector below if
			// valid, else a nullptr

			auto velocities_ptr = ToArray(vlp);

			if (velocities_ptr)
			{
				if (velocities_ptr->size() > 0)
				{
					// Walk through the array (vector)
					for (auto vlayer_sh_ptr : *velocities_ptr)
					{
						VerifyVelocityLayer(zzz, vlayer_sh_ptr);
					}
				}
				else
				{
					LogError(zzz, DfxVerifyResult::VelocitiesMustBeNonEmptyArray);
				}
			}
			else
			{
				LogError(zzz, DfxVerifyResult::VelocitiesMustBeNonEmptyArray);
			}

		}
		else
		{
			LogError(zzz, DfxVerifyResult::VelocitiesMissing);
		}

		return errcnt == save_errcnt;
	}

	bool DfxParser::VerifyVelocityLayer(const std::string zzz, std::shared_ptr<Value>& vlayer_sh_ptr)
	{
		int save_errcnt = errcnt;

		auto corby = ToNameValue(vlayer_sh_ptr);

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
						LogError(zzz, DfxVerifyResult::InvalidVelocityCode);
						break;
					}
					++p;
				}
			}
			else
			{
				LogError(zzz, DfxVerifyResult::InvalidVelocityCode);
			}

			// Okay, regardless of whether we have a valid velocity code,
			// let's check the body to see if it's kosher.

			// First off, it must be a {}-list

			auto& vlayer_body = corby->pair.second;

			auto vlayer_body_map_ptr =  ToObjectMap(vlayer_body);

			if (vlayer_body_map_ptr)
			{
				auto new_zzz = zzz + '/' + vel_code_str;

				// Might have an optional path

				bool path_must_be_specified = false;
				VerifyPath(new_zzz, vlayer_body_map_ptr, path_must_be_specified);

				// MUST have a non-empty robins array

				VerifyRobins(new_zzz, vlayer_body_map_ptr);
			}
			else
			{
				// We shouldn't get here, seein's how we already checked for
				// name value earlier, so vlayer_body *will* be value
				LogError(zzz, DfxVerifyResult::VelocityMustBeNameValue);
			}
		}
		else
		{
			LogError(zzz, DfxVerifyResult::VelocityMustBeNameValue);
		}

		return errcnt == save_errcnt;
	}


	bool DfxParser::VerifyRobins(const std::string zzz, const object_map_type* parent_map_ptr)
	{
		int save_errcnt = errcnt;

		auto robins_arr_ptr = GetArrayProperty(parent_map_ptr, "robins");

		if (robins_arr_ptr)
		{
			auto nrobins = robins_arr_ptr->size();
			if (nrobins > 0)
			{
				// Walk thru the robins. Each robin is
				// represented as a name-value pair.

				for (auto robin_sh_ptr : *robins_arr_ptr)
				{
					auto robin_nv_ptr  = ToNameValue(robin_sh_ptr);

					if (robin_nv_ptr)
					{
						VerifyRobin(zzz, robin_nv_ptr);
					}
					else
					{
						LogError(zzz, DfxVerifyResult::RobinMustBeNameValue);
					}
				}
			}
			else
			{
				LogError(zzz, DfxVerifyResult::RobinsMustBeNonEmptyArray);
			}
		}
		else
		{
			LogError(zzz, DfxVerifyResult::RobinsMustBeNonEmptyArray);
		}

		return errcnt == save_errcnt;

	}

	bool DfxParser::VerifyRobin(const std::string zzz, std::shared_ptr<NameValue> &robin_nv_ptr)
	{
		int save_errcnt = errcnt;

		// The name of a robin should be a valid file name (no paths please)

		auto& fname = robin_nv_ptr->pair.first;

		// @@ TODO: Check for valid file name

		// Then:

		auto new_zzz = zzz + '/' + fname;

		// The body of a robin must be a {}-list having the following
		// three optional items: offset, peak, and rms.

		auto& robin_body = robin_nv_ptr->pair.second;

		auto robin_body_map_ptr = ToObjectMap(robin_body);

		if (robin_body_map_ptr)
		{
			bool must_be_specified = false;
			VerifyOffset(new_zzz, robin_body_map_ptr, must_be_specified);
			VerifyPeak(new_zzz, robin_body_map_ptr, must_be_specified);
			VerifyRMS(new_zzz, robin_body_map_ptr, must_be_specified);

			auto peak_opt = GetSimpleProperty(robin_body_map_ptr, "peak");
			auto rms_opt = GetSimpleProperty(robin_body_map_ptr, "rms");
		}
		else
		{
			// Should never reach here, cause we alerady verified earlier
			// it's a name value
			LogError(zzz, DfxVerifyResult::RobinMustBeNameValue);
		}

		return errcnt == save_errcnt;
	}

	bool DfxParser::VerifyOffset(const std::string zzz, const object_map_type* parent_map, bool must_be_specified)
	{
		int save_errcnt = errcnt;

		// Check for a possibly optional offset.
		// If we find such a property, we make sure the value it's a whole number.

		auto vp = PropertyExists(parent_map, "offset");

		if (vp)
		{
			// We found the property. Is it the right type?
			// Is it a whole number?

			auto svp = ToSimpleValue(vp);

			if (svp)
			{
				// Okay, it's a simple value. Is it a whole number?

				if (svp->tkn.type == TokenEnum::WholeNumber)
				{
				}
				else
				{
					LogError(zzz, DfxVerifyResult::OffsetMustBeWholeNumber);
				}
			}
			else
			{
				LogError(zzz, DfxVerifyResult::OffsetMustBeWholeNumber);
			}
		}
		else
		{
			if (must_be_specified)
			{
				LogError(zzz, DfxVerifyResult::OffsetMissing);
			}
		}

		return errcnt == save_errcnt;
	}

	bool DfxParser::VerifyPeak(const std::string zzz, const object_map_type* parent_map, bool must_be_specified)
	{
		int save_errcnt = errcnt;

		// Check for a possibly optional peak specification.
		// If we find such a property, we make sure the value it's a number, 
		// whole or floating point doesn't matter. It may have an optional 
		// db unit on it.

		auto vp = PropertyExists(parent_map, "peak");

		if (vp)
		{
			// We found the property. Is it the right type?
			// Is it a number?

			auto svp = ToSimpleValue(vp);

			if (svp)
			{
				// Okay, it's a simple value. Is it a number?

				if (is_Bryx_number(svp->type))
				{
					// Okay, it's a number. If it's a ratio or a simple floating point number, then it must be 0 < rat <= 1.0.
					// If it's in db, it must be negative.

					VerifyWavePropertyRatio(zzz + '/' + "peak", svp->tkn);
				}
				else
				{
					LogError(zzz, DfxVerifyResult::PeakMustBeNumber);
				}
			}
			else
			{
				LogError(zzz, DfxVerifyResult::PeakMustBeNumber);
			}
		}
		else
		{
			if (must_be_specified)
			{
				LogError(zzz, DfxVerifyResult::PeakMissing);
			}
		}

		return errcnt == save_errcnt;
	}

	bool DfxParser::VerifyRMS(const std::string zzz, const object_map_type* parent_map, bool must_be_specified)
	{
		int save_errcnt = errcnt;

		// Check for a possibly optional rms specification.
		// If we find such a property, we make sure the value it's a number, 
		// whole or floating point doesn't matter. It may have an optional 
		// db unit on it.

		auto vp = PropertyExists(parent_map, "rms");

		if (vp)
		{
			// We found the property. Is it the right type?
			// Is it a number?

			auto svp = ToSimpleValue(vp);

			if (svp)
			{
				// Okay, it's a simple value. Is it a number?

				if (is_Bryx_number(svp->type))
				{
					// Okay, it's a number. If it's a ratio or a simple floating point number, then it must be 0 < rat <= 1.0.
					// If it's in db, it must be negative.

					VerifyWavePropertyRatio(zzz + '/' + "rms", svp->tkn);
				}
				else
				{
					LogError(zzz, DfxVerifyResult::RmsMustBeNumber);
				}
			}
			else
			{
				LogError(zzz, DfxVerifyResult::RmsMustBeNumber);
			}
		}
		else
		{
			if (must_be_specified)
			{
				LogError(zzz, DfxVerifyResult::RmsMissing);
			}
		}

		return errcnt == save_errcnt;
	}

	bool DfxParser::VerifyWavePropertyRatio(const std::string zzz, const Token& tkn)
	{
		// Here, we know it's a number. But is it a properly formed number
		// that would serve as a wave file's peak or rms value? In these
		// cases the value must be 0 < val <= 1.0.
		// To verify this, we must account for units like "x", "X", "%", 
		// and "db", "dB".

		// NOTE: When building kit for real, we anticipate using the EngrNum
		// class, which preserves the mantissa for round tripping.

		int save_errcnt = errcnt;

		auto& traits = tkn.number_traits;

		if (traits.HasExponent() || traits.HasMetricPrefix())
		{
			LogError(zzz, DfxVerifyResult::ValueNotLegal);
		}
		else
		{
			if (traits.HasGenericUnits())
			{
				LogError(zzz, DfxVerifyResult::ValueHasWrongUnits);
			}
			else
			{
				// Okay, we have no exponents to deal with, but we may still have
				// ratio units.

				double num = 0.0;
				bool past_decimal_point = false;
				double fact = 1.0;

				int col = 0;
				int ecol = traits.ratio_units_locn > -1 ? traits.ratio_units_locn : traits.end_locn;

				auto p = tkn.text.c_str();
				auto q = p;

				int sign = 1;

				if (*p == '-')
				{
					sign = -1;
					++p;
					++col;
				}
				else if (*p == '+')
				{
					sign = 1;
					++p;
					++col;
				}

				while (col < ecol)
				{
					if (*p == '.')
					{
						past_decimal_point = true;
					}
					else
					{
						int n = *p - '0'; // ascii only here!

						if (past_decimal_point)
						{
							fact *= 0.1;
							num += fact * n;
						}
						else
						{
							num = num * 10.0 + n;
						}

					}

					++col; ++p;
				}

				// examine number in light of units given

				if (traits.ratio_units_locn > -1)
				{
					// We have some type of ratio units, db, dB, x, X or %

					auto first_ch = q + traits.ratio_units_locn;

					if (*first_ch == 'x' || *first_ch == 'X')
					{
						// Ratio must no be negative, and must be <= 1.0;

						if (sign == -1 || num > 1.0)
						{
							LogError(zzz, DfxVerifyResult::ValueNotLegal);
						}
					}
					else if (*first_ch == '%')
					{
						// percentage must not be negative, and must be <= 100

						if (sign == -1 || num > 100.0)
						{
							LogError(zzz, DfxVerifyResult::ValueNotLegal);
						}
					}
					else if (*first_ch == 'd')
					{
						// In decibels. So number *must* *be* negative, or 0.

						if (sign == -1)
						{
							// okay
						}
						else if (num == 0.0)
						{
							// okay
						}
						else
						{
							LogError(zzz, DfxVerifyResult::ValueNotLegal);
						}

					}
				}
				else
				{
					// No units given, so the number must not be negative
					// and must be <= 1.0;

					if (sign == -1 || num > 1.0)
					{
						LogError(zzz, DfxVerifyResult::ValueNotLegal);
					}
				}
			}
		}

		return save_errcnt == errcnt;
	}

	// //////////////////////////////////////////////////////////////////


	/////////////////////////////////////////////////////////////////////


	void DfxParser::WriteFont(std::ostream& sout)
	{
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

	DfxVerifyResult DfxParser::LogError(const std::string prop, DfxVerifyResult err)
	{
		std::ostream & sl = *slog;
		sl << "In property " << prop << ": ";
		sl << to_string(err) << std::endl;
		++errcnt;
		return err;
	}

	void DfxParser::EndLog()
	{

	}

}
