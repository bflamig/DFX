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
 * into Json files that can be processed by any software supporting Json syntax.
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


#include <fstream>
#include "BryxParser.h"

namespace bryx
{
	enum class DfxVerifyResult
	{
		NoError,
		FileNotFound,
		InvalidFileType,
		MustBeSpecified,
		MustBeString,
		NoteMissing,
		NoteMustBeWholeNumber,
		KitsMissing,
		KitValWrongType,
		InstrumentsMissing,
		InstrumentsMustBeList,
		DrumValMustBeList,
		VelocitiesMissing,
		VelocitiesMustBeNonEmptyArray,
		VelocityMustBeNameValue,
		InvalidVelocityCode,      // Must start with v, the rest must be a whole number 0-127
		RobinsMissing,
		RobinsMustBeNonEmptyArray,
		RobinMustBeNameValue,
		RobinNameMustBeValidPath,
		OffsetMustBeWholeNumber,
		OffsetMissing, // We put this here in case we decide it's not optional
		PeakMustBeNumber,
		PeakMissing, // We put this here in case we decide it's not optional
		RmsMustBeNumber,
		RmsMissing, // We put this here in case we decide it's not optional
		ValueHasWrongUnits,
		ValueNotLegal,
		UnspecifiedError
	};

	extern std::string to_string(DfxVerifyResult result);

	class DfxVerifyResultPkg {
	public:
		std::string msg;
		DfxVerifyResult code;
	public:
		DfxVerifyResultPkg();
		DfxVerifyResultPkg(std::string msg_, DfxVerifyResult code_);
	public:
		void Clear();
		void ResetMsg();
		void Print(std::ostream& sout) const;
	};

	// ////////////////////////////////////////////////////////////////////
	// 
	//
	//
	// ////////////////////////////////////////////////////////////////////


	class DfxParser : public Parser {
	public:

		std::ostream* slog;
		std::string sound_font_path; // Filled in at load time.

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

		ParserResult LoadFile(std::string fname);

	public:

		const object_map_type* GetKits() const
		{
			return root_map;
		}

		size_t NumKits() const
		{
			return root_map->size();
		}

		int NumErrs() const
		{
			return errcnt;
		}

#if 0

		// Not well thought out yet, so we comment this out

		const object_map_type* GetKit(std::string name) const
		{
			const object_map_type* omp = GetObjectProperty(root_map, name);
			return omp;
		}

		const std::optional<std::string> GetRelPath(const object_map_type* kit_map_ptr) const
		{
			// Returns "" string if no path found

			auto p = GetSimpleProperty(kit_map_ptr, "path");

			return p;
		}

		const object_map_type* GetInstruments(const object_map_type* kit) const
		{
			return GetObjectProperty(kit, "instruments"); 
		}

		const object_map_type* GetInstrument(const object_map_type* instruments, std::string name) const
		{
			// Really need to wrap these with try-catches, because at() throws
			// an exception if thingy not found.
			try
			{
				auto freaky = instruments->at(name);
				return ToObjectMap(freaky);
			}
			catch (...)
			{
				return nullptr;
			}
		}

#endif

	public:

		bool Verify();
		bool VerifyKit(const std::string zzz, const nv_type& kit);
		bool VerifyPath(const std::string zzz, const object_map_type* parent_map, bool path_must_be_specified = false);
		bool VerifyInstruments(const std::string zzz, const object_map_type* instrument_map_ptr);
		bool VerifyInstrument(const std::string zzz, const nv_type& drum_nv);
		bool VerifyNote(const std::string zzz, const object_map_type* parent_map, bool note_must_be_specified);
		bool VerifyVelocityLayers(const std::string zzz, const object_map_type* parent_map);
		bool VerifyVelocityLayer(const std::string zzz, std::shared_ptr<Value>& vlayer_sh_ptr);
		bool VerifyRobins(const std::string zzz, const object_map_type* parent_map_ptr);
		bool VerifyRobin(const std::string zzz, std::shared_ptr<NameValue>& robin_nv_ptr);
		bool VerifyFname(const std::string zzz, const object_map_type* parent_map, bool must_be_specified);
		bool VerifyOffset(const std::string zzz, const object_map_type* parent_map, bool offset_must_be_specified);
		bool VerifyPeak(const std::string zzz, const object_map_type* parent_map, bool peak_must_be_specified);
		bool VerifyRMS(const std::string zzz, const object_map_type* parent_map, bool rms_must_be_specified);
		bool VerifyWavePropertyRatio(const std::string zzz, const Token& tkn);

	public:

		void StartLog(std::ostream& slog);
		DfxVerifyResult LogError(const std::string prop, DfxVerifyResult err);
		void EndLog();

		void WriteDfx(std::ostream& sout);
		void WriteDfx(std::ostream& sout, SyntaxModeEnum synmode);

	};

}; // end of namespace