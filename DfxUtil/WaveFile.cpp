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

#include "WaveFile.h"

namespace dfx
{
	WaveFile::WaveFile()
	: errors()
	{
		Clear();
	}

	WaveFile::~WaveFile()
	{
		Close();
	}

	void WaveFile::Clear(bool but_not_errors)
	{
		if (!but_not_errors)
		{
			errors.clear();
		}

		fileName = "";
		fd = 0;
		byteswap = false;
		isWaveFile = false;
		fileFrames = 0;
		dataOffset = 0;
		nChannels = 0;
		dataType = SampleFormat::SINT16;
		fileRate = 0.0;
	}

	void WaveFile::LogError(AudioResult err, const std::string_view& msg)
	{
		errors.push_back(AudioResultPkg(msg, err));
	}

	void WaveFile::LogError(AudioResult err, const std::stringstream& msg)
	{
		errors.push_back(AudioResultPkg(msg.str(), err));
	}

	const AudioResultPkg WaveFile::LastError() const
	{
		auto n = errors.size();

		if (n == 0)
		{
			return AudioResultPkg();
		}
		else
		{
			return errors.back();
		}
	}

	void WaveFile::Close()
	{
		if (fd)
		{
			fclose(fd);
		}

		bool but_not_errors = true;
		Clear(but_not_errors);
	}

	bool WaveFile::CheckBoundarySanity(unsigned proposedStartFrame, unsigned proposedEndFrame)
	{
		if (proposedEndFrame >= fileFrames)
		{
			std::stringstream msg;
			msg << "endFrame argument " << proposedEndFrame << " is >= file size " << fileFrames;
			LogError(AudioResult::FUNCTION_ARGUMENT, msg);
			return false;
		}

		// If proposed end frame is 0, it means "till the end of file"

		unsigned buffEnd = proposedEndFrame > 0 ? proposedEndFrame : fileFrames;

		if (proposedStartFrame >= buffEnd)
		{
			std::stringstream msg;
			msg << "startFrame argument " << proposedStartFrame << " is >= virtual file size " << buffEnd;
			LogError(AudioResult::FUNCTION_ARGUMENT, msg);
			return false;
		}

		return true;
	}

	bool WaveFile::OpenForReading(const std::string_view& fileName_)
	{
		// WARNING! Do not use for StkRaw files.

		Close(); // If already open, close what we got

		fileName = fileName_;

		// Try to open the file.
#ifdef __OS_WINDOWS__ // @@ BRY
		errno_t et = fopen_s(&fd, fileName.c_str(), "rb");
#else
		fd = fopen(fileName.c_str(), "rb");
#endif
		if (!fd)
		{
			std::stringstream msg;
			msg << "could not open or find file (" << fileName << ")";
			LogError(AudioResult::FILE_NOT_FOUND, msg);
			return false;
		}

		// header (8 + 4 bytes):

		//byte[] riffId = reader.ReadBytes(4);    // "RIFF"
		//int fileSize = reader.ReadInt32();      // size of entire file
		//byte[] typeId = reader.ReadBytes(4);    // "WAVE"

		// Attempt to determine file type from header
		bool result = false;

		char header[12];
		if (fread(&header, sizeof(header), 1, fd) != 1)
		{
			std::stringstream msg;
			msg << "problem reading header for file (" << fileName << ") on open";
			LogError(AudioResult::FILE_ERROR, msg);
			return false;
		}

		int fileSize = 0;

		if (!memcmp(header, "RIFF", 4) && !memcmp(&header[8], "WAVE", 4)) // // @@ was strncmp, but to shut up compiler ...
		{
			result = getWavInfo();
		}
		else
		{
			std::stringstream msg;
			msg << "not a wave file (" << fileName << ")";
			LogError(AudioResult::FILE_ERROR, msg);
			return false;
		}

		return result;

	}

	bool WaveFile::getWavInfo()
	{
		bool good_tag = false; // Used later

		// Find "format" chunk ... it must come before the "data" chunk.

		char id[4];
		int32_t chunkSize;

		if (fread(&id, 4, 1, fd) != 1) goto error;

		while (memcmp(id, "fmt ", 4)) // @@ was strncmp, but to shut up compiler ...
		{
			if (fread(&chunkSize, 4, 1, fd) != 1) goto error;

#ifndef __LITTLE_ENDIAN__
			swap(&chunkSize);
#endif
			if (fseek(fd, chunkSize, SEEK_CUR) == -1) goto error;
			if (fread(&id, 4, 1, fd) != 1) goto error;
		}

		// Check that the data is not compressed.

		unsigned short format_tag;
		if (fread(&chunkSize, 4, 1, fd) != 1) goto error; // Read fmt chunk size.
		if (fread(&format_tag, 2, 1, fd) != 1) goto error;

#ifndef __LITTLE_ENDIAN__
		swap(&format_tag);
		swap(&chunkSize);
#endif
		if (format_tag == 0xFFFE)
		{
			// WAVE_FORMAT_EXTENSIBLE

			dataOffset = ftell(fd);

			if (fseek(fd, 14, SEEK_CUR) == -1) goto error;

			unsigned short extSize;
			if (fread(&extSize, 2, 1, fd) != 1) goto error;

#ifndef __LITTLE_ENDIAN__
			swap(&extSize);
#endif
			if (extSize == 0) goto error;
			if (fseek(fd, 6, SEEK_CUR) == -1) goto error;

			if (fread(&format_tag, 2, 1, fd) != 1) goto error;

#ifndef __LITTLE_ENDIAN__
			swap(&format_tag);
#endif
			if (fseek(fd, dataOffset, SEEK_SET) == -1) goto error;
		}
		if (format_tag != 1 && format_tag != 3)
		{
			// PCM = 1, FLOAT = 3
			std::stringstream msg;
			msg << "file contains unsupported format tag: " << format_tag << " in getWavInfo() for (" << fileName << ")";
			LogError(AudioResult::FILE_ERROR, msg);
			return false;
		}

		// Get number of channels from the header.
		int16_t temp;
		if (fread(&temp, 2, 1, fd) != 1) goto error;

#ifndef __LITTLE_ENDIAN__
		swap(&temp);
#endif
		nChannels = static_cast<unsigned>(temp);

		// Get file sample rate from the header.

		int32_t srate;
		if (fread(&srate, 4, 1, fd) != 1) goto error;

#ifndef __LITTLE_ENDIAN__
		swap(&srate);
#endif
		fileRate = (double)srate;

		// Determine the data type.

		dataType = SampleFormat::SINT16; // Just a default to shut up compiler

		if (fseek(fd, 6, SEEK_CUR) == -1) goto error;   // Locate bits_per_sample info.
		if (fread(&temp, 2, 1, fd) != 1) goto error;

#ifndef __LITTLE_ENDIAN__
		swap(&temp);
#endif
		if (format_tag == 1)
		{
			//if (temp == 8)
			//{
			//	dataType = SampleFormat::SINT8;
			//	good_tag = true;
			//}
			if (temp == 16)
			{
				dataType = SampleFormat::SINT16;
				good_tag = true;
			}
			else if (temp == 24)
			{
				dataType = SampleFormat::SINT24;
				good_tag = true;
			}
			else if (temp == 32)
			{
				dataType = SampleFormat::SINT32;
				good_tag = true;
			}
		}
		else if (format_tag == 3)
		{
			if (temp == 32)
			{
				dataType = SampleFormat::FLOAT32;
				good_tag = true;
			}
			else if (temp == 64)
			{
				dataType = SampleFormat::FLOAT64;
				good_tag = true;
			}
		}

		if (!good_tag)
		{
			// UNSUPPORTED TYPE OR COULDN'T DETERMINE

			std::stringstream msg;
			msg << temp << "bits per sample with data format tag " << format_tag << " not supported in getWavInfo() for (" << fileName << ")";
			LogError(AudioResult::FILE_ERROR, msg);
			return false;
		}

		// Jump over any remaining part of the "fmt" chunk.
		if (fseek(fd, chunkSize - 16, SEEK_CUR) == -1) goto error;

		// Find "data" chunk ... it must come after the "fmt" chunk.
		if (fread(&id, 4, 1, fd) != 1) goto error;

		while (memcmp(id, "data", 4)) // @@ was strncmp, but to shut up compiler ...
		{
			if (fread(&chunkSize, 4, 1, fd) != 1) goto error;

#ifndef __LITTLE_ENDIAN__
			swap(&chunkSize);
#endif
			chunkSize += chunkSize % 2; // chunk sizes must be even
			if (fseek(fd, chunkSize, SEEK_CUR) == -1) goto error;
			if (fread(&id, 4, 1, fd) != 1) goto error;
		}

		// Get length of data from the header.
		int32_t bytes;

		if (fread(&bytes, 4, 1, fd) != 1) goto error;

#ifndef __LITTLE_ENDIAN__
		swap(&bytes);
#endif
		fileFrames = bytes / temp / nChannels;  // sample frames
		fileFrames *= 8;  // sample frames  // @@ ?? WHAT?

		dataOffset = ftell(fd);

#ifndef __LITTLE_ENDIAN__
		byteswap = true;
#else
		byteswap = false;
#endif

		isWaveFile = true;
		return true;

	error:
		std::stringstream msg;
		msg << "unspecified problem when reading file (" << fileName << ")";
		LogError(AudioResult::FILE_ERROR, msg);
		return false;
	}

	// //////////////////////////////////////////////////////////////////////////
	// Adapted from Stk code
	//

	// WAV header structure. See
	// http://www-mmsp.ece.mcgill.ca/documents/audioformats/WAVE/Docs/rfc2361.txt
	// for information regarding format codes.

	struct WaveHeader 
	{
		char riff[4];            // "RIFF"
		int32_t fileSize;        // in bytes
		char wave[4];            // "WAVE"
		char fmt[4];             // "fmt "
		int32_t chunkSize;       // in bytes (16 for PCM)
		int16_t formatCode;      // 1=PCM, 2=ADPCM, 3=IEEE float, 6=A-Law, 7=Mu-Law
		int16_t nChannels;       // 1=mono, 2=stereo
		int32_t sampleRate;
		int32_t bytesPerSecond;
		int16_t bytesPerSample;  // 2=16-bit mono, 4=16-bit stereo
		int16_t bitsPerSample;
		int16_t cbSize;          // size of extension
		int16_t validBits;       // valid bits per sample
		int32_t channelMask;     // speaker position mask
		char subformat[16];      // format code and GUID
		char fact[4];            // "fact"
		int32_t factSize;        // fact chunk size
		int32_t frames;          // sample frames
	};


	// These return true if they succeed

	inline bool parm_write(FILE* fd, const std::string_view& txt)
	{
		return fwrite(txt.data(), txt.size(), 1, fd) == 1;
	}

	inline bool parm_write(FILE* fd, uint32_t v)
	{
		return fwrite(&v, sizeof(v), 1, fd) == 1;  
	}

	inline bool parm_write(FILE* fd, int32_t v)
	{
		return fwrite(&v, sizeof(v), 1, fd) == 1;
	}

	inline bool parm_write(FILE* fd, int16_t v)
	{
		return fwrite(&v, sizeof(v), 1, fd) == 1;
	}

	bool WaveFile::OpenForWriting(const std::string_view& fileName_, SampleFormat dataType_, int nChannels_, int sampleRate_)
	{
		// WARNING! Do not use for StkRaw files.

		Close(); // If already open, close what we got

		fileName = fileName_;
		dataType = dataType_;
		nChannels = nChannels_;
		fileRate = sampleRate_;
		isWaveFile = true;
		dataOffset = 0;

		// Try to open the file.
#ifdef __OS_WINDOWS__ // @@ BRY
		errno_t et = fopen_s(&fd, fileName.c_str(), "wb");
#else
		fd = fopen(fileName.c_str(), "wb");
#endif
		if (!fd)
		{
			std::stringstream msg;
			msg << "could not create file (" << fileName << ")";
			LogError(AudioResult::FILE_ERROR, msg);
			return false;
		}

		WaveHeader hdr = 
		{ {'R','I','F','F'}, 44, {'W','A','V','E'}, {'f','m','t',' '}, 16, 1, 1,
		  (int32_t)fileRate, 0, 2, 16, 0, 0, 0,
		  {'\x01','\x00','\x00','\x00','\x00','\x00','\x10','\x00','\x80','\x00','\x00','\xAA','\x00','\x38','\x9B','\x71'},
		  {'f','a','c','t'}, 4, 0 
		};

		hdr.nChannels = (int16_t)nChannels;

		switch (dataType)
		{
			case SampleFormat::SINT16:
			{
				hdr.bitsPerSample = 16;
			}
			break;

			case SampleFormat::SINT24:
			{
				hdr.bitsPerSample = 24;
			}
			break;

			case SampleFormat::SINT32:
			{
				hdr.bitsPerSample = 32;
			}
			break;

			case SampleFormat::FLOAT32:
			{
				hdr.formatCode = 3;
				hdr.bitsPerSample = 32;
			}
			break;

			case SampleFormat::FLOAT64:
			{
				hdr.formatCode = 3;
				hdr.bitsPerSample = 64;
			}
			break;

		}

		hdr.bytesPerSample = (int16_t) (nChannels * hdr.bitsPerSample / 8);  // @@ TODO: Really means bytesPerFrame!. Maybe change header name.
		hdr.bytesPerSecond = (int32_t) (hdr.sampleRate * hdr.bytesPerSample);

		unsigned bytesToWrite = 36;

		if (nChannels > 2 || hdr.bitsPerSample > 16) // use extensible format
		{
			bytesToWrite = 72;
			hdr.chunkSize += 24;

#pragma warning( push )
#pragma warning( disable : 4309 )
			hdr.formatCode = 0xFFFE;   // VS compiler flags this as a truncation. @@ BRY worked around warning
#pragma warning( pop )

			hdr.cbSize = 22;
			hdr.validBits = hdr.bitsPerSample;
			int16_t* subFormat = (int16_t*)&hdr.subformat[0];

			if (dataType == SampleFormat::FLOAT32 || dataType == SampleFormat::FLOAT64)
			{
				*subFormat = 3;
			}
			else *subFormat = 1;
		}

		byteswap = false;

#ifndef __LITTLE_ENDIAN__
		byteswap = true;
		swap(&hdr.chunkSize);
		swap(&hdr.formatCode);
		swap(&hdr.nChannels);
		swap(&hdr.sampleRate);
		swap(&hdr.bytesPerSecond);
		swap(&hdr.bytesPerSample);
		swap(&hdr.bitsPerSample);
		swap(&hdr.cbSize);
		swap(&hdr.validBits);
		swap((int16_t*)(&hdr.subformat[0]));
		swap(&hdr.factSize);
#endif

		if (fwrite(&hdr, 1, bytesToWrite, fd) != bytesToWrite) goto error;

		if (!parm_write(fd, "data")) goto error;
		if (!parm_write(fd, (int32_t)0)) goto error;

		return true;

	error:
		std::stringstream msg;
		msg << "problem creating wav file (" << fileName << ")";
		LogError(AudioResult::FILE_ERROR, msg);
		return false;
	}


	bool WaveFile::BackPatchAfterWrite(SampleFormat dataType, unsigned frameCounter)
	{
		int bytesPerSample = 0;

		switch (dataType)
		{
			case SampleFormat::SINT16:
			{
				bytesPerSample = 2;
			}
			break;
			case SampleFormat::SINT24:
			{
				bytesPerSample = 3;
			}
			break;
			case SampleFormat::SINT32:
			case SampleFormat::FLOAT32:
			{
				bytesPerSample = 4;
			}
			break;
			case SampleFormat::FLOAT64:
			{
				bytesPerSample = 8;
			}
			break;
		}

		if (bytesPerSample == 0)
		{
			std::stringstream msg;
			msg << "Unsupported data type in BackPatchAfterWrite() for (" << fileName << ")";
			LogError(AudioResult::FILE_ERROR, msg);
			return false;
		}

		bool useExtensible = false;
		int dataLocation = 40;

		if (bytesPerSample > 2 || nChannels > 2) 
		{
			useExtensible = true;
			dataLocation = 76;
		}

		uint32_t nbytes = (uint32_t)(frameCounter * nChannels * bytesPerSample);

		if (nbytes % 2) // pad extra byte if odd
		{ 
			char sample = 0;
			if (fwrite(&sample, sizeof(sample), 1, fd) != 1) goto error;
		}

#ifndef __LITTLE_ENDIAN__
		swap(&nbytes);
#endif
		fseek(fd, dataLocation, SEEK_SET); // jump to data length
		if (fwrite(&nbytes, sizeof(nbytes), 1, fd) != 1) goto error;

		nbytes += 44;

		if (useExtensible) nbytes += 36;

#ifndef __LITTLE_ENDIAN__
		swap(&nbytes);
#endif
		fseek(fd, 4, SEEK_SET); // jump to file size
		if (fwrite(&nbytes, sizeof(nbytes), 1, fd) != 1) goto error;

		if (useExtensible) 
		{ 
			// fill in the "fact" chunk frames value
			nbytes = (uint32_t)frameCounter;

#ifndef __LITTLE_ENDIAN__
			swap(&nbytes);
#endif
			fseek(fd, 68, SEEK_SET);
			if (fwrite(&nbytes, sizeof(nbytes), 1, fd) != 1) goto error;
		}

		return true;

	error:
		std::stringstream msg;
		msg << "problem backpatching wav file (" << fileName << ")";
		LogError(AudioResult::FILE_ERROR, msg);
		return false;
	}

} // end of namespace