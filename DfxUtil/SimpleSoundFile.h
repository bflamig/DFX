#pragma once

// 
// Simple Sound File:
//
// Simple read/write support for wav files. And that's it!
//
// 

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
	// This class ONLY handles wav files. 
	// It supports writing though, too.

	class SimpleSoundFile {
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

		SimpleSoundFile();
		virtual ~SimpleSoundFile();

		//SoundFile(const SoundFile& other);
		//SoundFile(SoundFile&& other) noexcept;

		//void operator=(const SoundFile& other);
		//void operator=(SoundFile&& other) noexcept;

		void Clear(bool but_not_errors = false);

		bool OpenForReading(const std::string_view& fileName_);
		bool getWavInfo();

		// Low level
		bool OpenForWriting(const std::string_view& fileName_, SampleFormat dataType_, int nChannels_, int sampleRate_);

		// Higher level

		bool OpenForWriting(const std::string_view& fileName_, FrameBuffer<int16_t>& buffer)
		{
			return OpenForWriting(fileName_, SampleFormat::SINT16, buffer.nChannels, (int)buffer.dataRate);
		}

		bool OpenForWriting(const std::string_view& fileName_, FrameBuffer<int24_t>& buffer)
		{
			return OpenForWriting(fileName_, SampleFormat::SINT24, buffer.nChannels, (int)buffer.dataRate);
		}

		bool OpenForWriting(const std::string_view& fileName_, FrameBuffer<int32_t>& buffer)
		{
			return OpenForWriting(fileName_, SampleFormat::SINT32, buffer.nChannels, (int)buffer.dataRate);
		}

		bool OpenForWriting(const std::string_view& fileName_, FrameBuffer<float>& buffer)
		{
			return OpenForWriting(fileName_, SampleFormat::FLOAT32, buffer.nChannels, (int)buffer.dataRate);
		}

		bool OpenForWriting(const std::string_view& fileName_, FrameBuffer<double>& buffer)
		{
			return OpenForWriting(fileName_, SampleFormat::FLOAT64, buffer.nChannels, (int)buffer.dataRate);
		}

		bool CheckBoundarySanity(unsigned proposedStartFrame, unsigned proposedEndFrame);

		template<class T> bool Read(FrameBuffer<T>& buffer, unsigned startFrame = 0, unsigned endFrame = 0)
		{
			// Unlike SoundFile::Read(), this version reads the data as is, does not try to retype or rescale.

			// Buffer is auto-sized here.

			// @@ NOTE: endFrame is *one past the end* of where we want. Don't forget this!
			// However, if endFrame comes in 0, it means to read from the start frame to the
			// end of the buffer, that is, all the frames that are available, starting with
			// the start frame. Since we are using base 0, then endFrame = fileFrames is by
			// defn one past the end of the last frame we'd like to read.

			// Make sure we have an open file.
			if (fd == 0)
			{
				std::stringstream msg;
				msg << "file not open (" << fileName << ").";
				LogError(AudioResult::FILE_ERROR, msg);
				return false;
			}

			if (!CheckBoundarySanity(startFrame, endFrame))
			{
				return false;
			}

			// endFrame == 0 is code for "to the end of the file"

			unsigned buffEnd = endFrame > 0 ? endFrame : fileFrames;

			unsigned nFrames = buffEnd - startFrame;

			buffer.Resize(nFrames, nChannels);

			long nSamples = (long)(nFrames * nChannels);
			unsigned long offset = startFrame * nChannels;

			T* dest_buffer = buffer.samples.get();

			if (fseek(fd, dataOffset + (offset * sizeof(T)), SEEK_SET) == -1) goto error;

			if (fread(dest_buffer, sizeof(T), nSamples, fd) != nSamples) goto error;

			if (byteswap)
			{
				byteSwapBuffer(dataType, dest_buffer, nSamples);
			}

			buffer.SetDataRate(fileRate);
			return true;

		error:

			std::stringstream msg;
			msg << "problem reading file (" << fileName << ")";
			LogError(AudioResult::FILE_ERROR, msg);
			return false;
		}

		template<class T> 
		bool Write(FrameBuffer<T>& buffer, unsigned startFrame = 0, unsigned endFrame = 0)
		{
			// @@ ASSUMES the file pointer is right where we want to start writing samples.

			// @@ NOTE: endFrame is *one past the end* of where we want. Don't forget this!
			// However, if endFrame comes in 0, it means to write from the start frame to the
			// end of the buffer, that is, all the frames that are available, starting with
			// the start frame. Since we are using base 0, then endFrame = fileFrames is by
			// defn one past the end of the last frame we'd like to write.

			if (buffer.nFrames == 0)
			{
				std::stringstream msg;
				msg << "buffer is empty when trying to write to (" << fileName << ").";
				LogError(AudioResult::FUNCTION_ARGUMENT, msg);
				return false;
			}

			unsigned buffEnd = endFrame > 0 ? endFrame : buffer.nFrames;

			// Make sure we have an open file.

			if (fd == 0)
			{
				std::stringstream msg;
				msg << "file not open (" << fileName << ").";
				LogError(AudioResult::FILE_ERROR, msg);
				return false;
			}

			bool sane = startFrame < endFrame;

			if (!(startFrame < buffEnd))
			{
				std::stringstream msg;
				msg << "Boundary arguments inconsistent: start (" << startFrame << " isn't < end (" << buffEnd << ")" << std::endl;
				LogError(AudioResult::FUNCTION_ARGUMENT, msg);
				return false;
				return false;
			}

			unsigned nFrames = buffEnd - startFrame; // Remember, buffEnd points to one past last frame to write, and we are in base 0.

			if (buffer.nFrames < nFrames)
			{
				std::stringstream msg;
				msg << "boundary arguments inconsistent with buffer size (" << fileName << ").";
				LogError(AudioResult::FUNCTION_ARGUMENT, msg);
				return false;
			}

			fileFrames = nFrames; // Fill in the sound file object member for good housekeeping.
			dataOffset = 0;       // Ditto. Not used for writing.

			unsigned nSamples = (unsigned)(nFrames * nChannels);
			unsigned start_offset = startFrame * nChannels;
			unsigned end_offset = buffEnd * nChannels;

			T* p = buffer.samples.get();
			p += start_offset;

			// We are ASSUMED to be pointing right where we want to write
			// if (fseek(fd, dataOffset + (start_offset * sizeof(T)), SEEK_SET) == -1) goto error;

			if (byteswap)
			{
				for (unsigned k = 0; k < nSamples; k++)
				{
					T *q = p++;
					swap(q);
					if (fwrite(q, sizeof(T), 1, fd) != 1) goto error;
				}
			}
			else
			{
				for (unsigned k = 0; k < nSamples; k++)
				{
					if (fwrite(p++, sizeof(T), 1, fd) != 1) goto error;
				}
			}

			return BackPatchAfterWrite(dataType, nFrames);

		error:

			std::stringstream msg;
			msg << "unspecified problem writing file (" << fileName << ")";
			LogError(AudioResult::FILE_ERROR, msg);
			return false;
		}

		bool BackPatchAfterWrite(SampleFormat dataType, unsigned frameCounter);

		void Close();

		bool isOpen() { return fd != 0; }

		void LogError(AudioResult err, const std::string_view& msg);
		void LogError(AudioResult err, const std::stringstream& msg);

		const AudioResultPkg LastError() const;
	};

} // end of namespace

