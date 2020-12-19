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
#include <cstdio>
#include <string>
#include <vector>
#include <sstream>
#include "FrameBuffer.h"
#include "AudioUtil.h"

namespace dfx
{
#if 0
	enum class AudioResult
	{
		NoError,
		STATUS,
		WARNING,
		DEBUG_PRINT,
		MEMORY_ALLOCATION,
		MEMORY_ACCESS,
		FUNCTION_ARGUMENT,
		FILE_NOT_FOUND,
		FILE_UNKNOWN_FORMAT,
		FILE_NAMING,        // @@ BRY ADDED THIS
		FILE_CONFIGURATION, // @@ BRY ADDED THIS
		FILE_ERROR,
		PROCESS_THREAD,
		PROCESS_SOCKET,
		PROCESS_SOCKET_IPADDR,
		AUDIO_SYSTEM,
		MIDI_SYSTEM,
		UNSPECIFIED
	};

	extern std::string to_string(AudioResult r);

	using AudioResultPkg = bryx::ResultPkg<AudioResult>;

#endif

	class SoundFile {
	public:

		std::vector<AudioResultPkg> errors;
		std::string fileName;

		FILE* fd;
		bool byteswap;
		bool isWaveFile;
		unsigned fileFrames;
		unsigned dataOffset;
		unsigned nChannels;
		SampleFormat dataType;
		double fileRate;

	public:

		SoundFile();
		virtual ~SoundFile();

		SoundFile(const SoundFile& other);
		SoundFile(SoundFile&& other) noexcept;

		void operator=(const SoundFile& other);
		void operator=(SoundFile&& other) noexcept;

		void Clear(bool but_not_errors = false);
		
		bool Open(const std::string_view &fileName_);
		bool OpenRaw(const std::string_view& fileName_, unsigned nChannels_, SampleFormat format_, double fileRate_);

		bool CheckBoundarySanity(unsigned proposedStartFrame, unsigned proposedEndFrame);

		// This version always converts to double, and maybe rescales so that 1 is full scale.
		bool Read(FrameBuffer<double>& buffer, unsigned startFrame, unsigned endFrame, bool use_1_fullscale);

		void Close();

		bool isOpen() { return fd != 0; }

		void LogError(AudioResult err, const std::string_view& msg);
		void LogError(AudioResult err, const std::stringstream& msg);

		const AudioResultPkg LastError() const;

	protected:

		bool getRawInfo(unsigned int nChannels_, SampleFormat format_, double FileRate_);
		bool getWavInfo();
		bool getSndInfo();
		bool getAifInfo();
		bool getMatInfo();
	};

} // end of namespace
