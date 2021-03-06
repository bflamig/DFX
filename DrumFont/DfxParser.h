#pragma once

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

//#include <fstream>
#include <filesystem>
#include "BryxParser.h"

namespace dfx
{
	using namespace bryx;

	enum class DfxResult
	{
		NoError,
		FileNotFound,
		FileOpenError,
		InvalidFileType,
		ParsingError,
		MustBeSpecified,
		MustBeString,
		NoteMissing,
		NoteMustBeWholeNumber,
		KitsMissing,
		KitValWrongType,
		InstrumentIncludeDataMissing,
		InstrumentsMissing,
		InstrumentsMustBeList,
		DrumValMustBeList,
		VelocitiesMissing,
		VelocitiesMustBeNonEmptySquareList,
		VelocityMustBeNameValue,
		InvalidVelocityCode,      // Must start with v, the rest must be a whole number 0-127
		RobinsMissing,
		RobinsMustBeNonEmptySquareList,
		RobinMustBeNameValue,
		RobinNameMustBeValidPath,
		BoundMustBeWholeNumber,
		BoundMissing, // We put this here in case we decide it's not optional
		PeakMustBeNumber,
		PeakMissing, // We put this here in case we decide it's not optional
		RmsMustBeNumber,
		RmsMissing, // We put this here in case we decide it's not optional
		ValueHasWrongUnits,
		ValueNotLegal,
		VerifyFailed,
		UnspecifiedError
	};

	extern std::string to_string(DfxResult result);

	using DfxResultPkg = ResultPkg<DfxResult>;

	// ////////////////////////////////////////////////////////////////////
	// 
	//
	//
	// ////////////////////////////////////////////////////////////////////

	class DfxParser : public Parser {
	public:

		std::ostream* slog;
		std::filesystem::path sound_font_path; // Filled in at load time.

		int errcnt; // Num errors during verify

	public:

		DfxParser()
		: Parser()
		, slog(0)
		, sound_font_path()
		, errcnt(0)
		{
		}

		explicit DfxParser(std::streambuf* sb_)
		: DfxParser()
		{
			SetStreamBuf(sb_);
		}

		virtual ~DfxParser() { }

	public:

		ParserResult LoadFile(std::ostream &serr, std::string_view &fname);
		DfxResult LoadAndVerify(std::ostream& serr, std::string_view& fname, bool as_include = false);

	public:

		const curly_list_type* GetKitsMapPtr() const
		{
			return root_map;
		}

		const curly_list_type* GetInstrumentIncludeMapPtr() const
		{
			return root_map;
		}

		const curly_list_type* GetInstrumentMapPtr(const curly_list_type* kit) const
		{
			return GetCurlyListProperty(kit, "instruments");
		}

		size_t NumKits() const
		{
			return root_map->size();
		}

		int NumErrs() const
		{
			return errcnt;
		}


	public:

		bool Verify();

		bool VerifyIncludeFile();

		bool VerifyKit(const std::string ctx, const nv_type& kit);
		bool VerifyPath(const std::string ctx, const curly_list_type* parent_map, bool path_must_be_specified = false);
		bool VerifyIncludeBasePath(const std::string ctx, const curly_list_type* parent_map, bool path_must_be_specified = false);
		bool VerifyInstruments(const std::string ctx, const curly_list_type* instrument_map_ptr);
		bool VerifyInstrument(const std::string ctx, const nv_type& drum_nv);
		bool VerifyNote(const std::string ctx, const curly_list_type* parent_map, bool note_must_be_specified);
		bool VerifyVelocityLayers(const std::string ctx, const curly_list_type* parent_map);
		bool VerifyVelocityLayer(const std::string ctx, std::shared_ptr<Value>& vlayer_sh_ptr);
		bool VerifyRobins(const std::string ctx, const curly_list_type* parent_map_ptr);
		bool VerifyRobin(const std::string ctx, std::shared_ptr<NameValue>& robin_nv_ptr);
		bool VerifyRobinBody(const std::string ctx, const curly_list_type* robin_body_map_ptr);
		bool VerifyFname(const std::string ctx, const curly_list_type* parent_map, bool must_be_specified);
		bool VerifyFname(const std::string ctx, std::shared_ptr<Value>& vp);
		bool VerifyStart(const std::string ctx, const curly_list_type* parent_map, bool start_must_be_specified);
		bool VerifyEnd(const std::string ctx, const curly_list_type* parent_map, bool end_must_be_specified);
		bool VerifyPeak(const std::string ctx, const curly_list_type* parent_map, bool peak_must_be_specified);
		bool VerifyRMS(const std::string ctx, const curly_list_type* parent_map, bool rms_must_be_specified);
		bool VerifyWaveMagnitude(const std::string ctx, const token_ptr& tkn);
		token_ptr ProcessAsNumber(const std::string ctx, std::shared_ptr<Value>& svp);

	public:

		void StartLog(std::ostream& slog);
		ParserResult LogError(const std::string ctx, ParserResult err);
		DfxResult LogError(const std::string prop, DfxResult err);
		DfxResult LogError(const std::string prop, DfxResult err, LexiResultPkg& err_pkg);
		void EndLog();

		void WriteDfx(std::ostream& sout);
		void WriteDfx(std::ostream& sout, SyntaxModeEnum synmode);

	};

}; // end of namespace