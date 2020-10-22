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
 * ****************************************************************************
 * NOTICE: This file is heavily derived from the FileRead function of the 
 * Synthesis ToolKit (STK):
 * 
	This class provides input support for various
	audio file formats.  Multi-channel (>2)
	soundfiles are supported.  The file data is
	returned via an external StkFrames object
	passed to the read() function.  This class
	does not store its own copy of the file data,
	rather the data is read directly from disk.

	FileRead currently supports uncompressed WAV,
	AIFF/AIFC, SND (AU), MAT-file (Matlab), and
	STK RAW file formats.  Signed integer (8-,
	16-, 24- and 32-bit) and floating-point (32- and
	64-bit) data types are supported.  Compressed
	data types are not supported.

	STK RAW files have no header and are assumed to
	contain a monophonic stream of 16-bit signed
	integers in big-endian byte order at a sample
	rate of 22050 Hz.  MAT-file data should be saved
	in an array with each data channel filling a
	matrix row.  The sample rate for MAT-files should
	be specified in a variable named "fs".  If no
	such variable is found, the sample rate is
	assumed to be 44100 Hz.

	by Perry R. Cook and Gary P. Scavone, 1995--2019.
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

#include <sstream>
#include "SoundFile.h"

namespace dfx
{
	std::string to_string(AudioResult r)
	{
		switch (r)
		{
			case AudioResult::NoError: return "everything fine";
			case AudioResult::STATUS: return "STATUS";
			case AudioResult::WARNING: return "WARNING";
			case AudioResult::DEBUG_PRINT: return "DEBUG_PRINT";
			case AudioResult::MEMORY_ALLOCATION: return "MEMORY_ALLOCATION";
			case AudioResult::MEMORY_ACCESS: return "MEMORY_ACCESS";
			case AudioResult::FUNCTION_ARGUMENT: return "FUNCTION_ARGUMENT";
			case AudioResult::FILE_NOT_FOUND: return "FILE_NOT_FOUND";
			case AudioResult::FILE_UNKNOWN_FORMAT: return "FILE_UNKNOWN_FORMAT";
			case AudioResult::FILE_NAMING: return "FILE_NAMING";
			case AudioResult::FILE_CONFIGURATION: return "FILE_CONFIGURATION";
			case AudioResult::FILE_ERROR: return "FILE_ERROR";
			case AudioResult::PROCESS_THREAD: return "PROCESS_THREAD";
			case AudioResult::PROCESS_SOCKET: return "PROCESS_SOCKET";
			case AudioResult::PROCESS_SOCKET_IPADDR: return "PROCESS_SOCKET_IPADDR";
			case AudioResult::AUDIO_SYSTEM: return "AUDIO_SYSTEM";
			case AudioResult::MIDI_SYSTEM: return "MIDI_SYSTEM";
			case AudioResult::UNSPECIFIED:
			default: return "UNSPECIFIED";
		}
	}


	SoundFile::SoundFile()
	: errors()
	{
		Clear();
	}

	SoundFile::SoundFile(const SoundFile& other)
	: errors(other.errors)
	, fileName(other.fileName)
	, fd(other.fd) // Both sharing pointer. Yikes!
	, byteswap(other.byteswap)
	, isWaveFile(other.isWaveFile)
	, fileFrames(other.fileFrames)
	, dataOffset(other.dataOffset)
	, nChannels(other.nChannels)
	, dataType(other.dataType)
	, fileRate(other.fileRate)
	{
		fd = {}; // Seems like a good idea?
	}

	SoundFile::SoundFile(SoundFile&& other) noexcept
	: errors(other.errors)
	, fileName(std::move(other.fileName))
	, fd(other.fd) // Both sharing pointer. Yikes!
	, byteswap(other.byteswap)
	, isWaveFile(other.isWaveFile)
	, fileFrames(other.fileFrames)
	, dataOffset(other.dataOffset)
	, nChannels(other.nChannels)
	, dataType(other.dataType)
	, fileRate(other.fileRate)
	{
		// jkuwmp :)

		other.errors = {};
		other.fd = {}; // Seems like a good idea ?
		other.byteswap = {};
		other.isWaveFile = {};
		other.fileFrames = {};
		other.dataOffset = {};
		other.nChannels = {};
		other.dataType = {};
		other.fileRate = {};
	}

	SoundFile::~SoundFile()
	{
		Close();
	}

	void SoundFile::operator=(const SoundFile& other)
	{
		if (this != &other)
		{
			errors = other.errors;
			fileName = other.fileName;
			fd = other.fd; // YIKES! NOT COOL (?)
			byteswap = other.byteswap;
			isWaveFile = other.isWaveFile;
			fileFrames = other.fileFrames;
			dataOffset = other.dataOffset;
			nChannels = other.nChannels;
			dataType = other.dataType;
			fileRate = other.fileRate;

			fd = {}; // Seems like a good idea (?)
		}
	}

	void SoundFile::operator=(SoundFile&& other) noexcept
	{
		if (this != &other)
		{
			errors = other.errors;
			fileName = std::move(other.fileName);
			fd = other.fd; // YIKES! NOT COOL (?)
			byteswap = other.byteswap;
			isWaveFile = other.isWaveFile;
			fileFrames = other.fileFrames;
			dataOffset = other.dataOffset;
			nChannels = other.nChannels;
			dataType = other.dataType;
			fileRate = other.fileRate;

			// jkuwmp :)

			other.errors = {};
			other.fd = {}; // Seems like a good idea ?
			other.byteswap = {};
			other.isWaveFile = {};
			other.fileFrames = {};
			other.dataOffset = {};
			other.nChannels = {};
			other.dataType = {};
			other.fileRate = {};
		}
	}


	void SoundFile::Clear()
	{
		errors.clear();
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

	void SoundFile::LogError(AudioResult err, const std::string_view& msg)
	{
		errors.push_back(AudioResultPkg(msg, err));
	}

	void SoundFile::LogError(AudioResult err, const std::stringstream& msg)
	{
		errors.push_back(AudioResultPkg(msg.str(), err));
	}

	void SoundFile::Close()
	{
		if (fd)
		{
			fclose(fd);
		}

		Clear();
	}

	bool SoundFile::Open(const std::string_view &fileName_)
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

		// Attempt to determine file type from header (unless RAW).
		bool result = false;

		char header[12];
		if (fread(&header, 4, 3, fd) != 3)
		{
			std::stringstream msg;
			msg << "problem reading header for file (" << fileName << ") on open";
			LogError(AudioResult::FILE_ERROR, msg);
			return false;
		}

		if (!memcmp(header, "RIFF", 4) && !memcmp(&header[8], "WAVE", 4)) // // @@ was strncmp, but to shut up compiler ...
		{
			result = getWavInfo();
		}
		else if (!memcmp(header, ".snd", 4)) // @@ was strncmp, but to shut up compiler ...
		{
			result = getSndInfo();
		}
		else if (!memcmp(header, "FORM", 4) &&  // @@ was strncmp, but to shut up compiler ...
			(!memcmp(&header[8], "AIFF", 4) || !memcmp(&header[8], "AIFC", 4)))
		{
			result = getAifInfo();
		}
		else
		{
			// Are we a MAT sound file?

			if (fseek(fd, 126, SEEK_SET) == -1)
			{
				std::stringstream msg;
				msg << "problem reading file (" << fileName << ") on open";
				LogError(AudioResult::FILE_ERROR, msg);
				return false;
			}

			if (fread(&header, 2, 1, fd) != 1)
			{
				std::stringstream msg;
				msg << "problem reading file (" << fileName << ") on open";
				LogError(AudioResult::FILE_ERROR, msg);
				return false;
			}

			if (!memcmp(header, "MI", 2) || !memcmp(header, "IM", 2)) // @@ was strncmp, but to shut up compiler ...
			{
				result = getMatInfo();
			}
			else
			{
				std::stringstream msg;
				msg << "problem reading file (" << fileName << ") on open";
				LogError(AudioResult::FILE_ERROR, msg);
				return false;
			}
		}

		// If here, we had a file type candidate but something else went wrong.
		if (result == false)
		{
			std::stringstream msg;
			msg << "problem reading file (" << fileName << ") on open";
			LogError(AudioResult::FILE_ERROR, msg);
			return false;
		}

		// Check for empty files.
		if (fileFrames == 0)
		{
			std::stringstream msg;
			msg << "file is empty upon open (" << fileName << ")";
			LogError(AudioResult::FILE_ERROR, msg);
			return false;
		}

		return true;
	}

	bool SoundFile::OpenRaw(const std::string_view& fileName_, unsigned nChannels_, SampleFormat format_, double fileRate_)
	{
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

		bool result = getRawInfo(nChannels_, format_, fileRate_);

		// If here, we had a file type candidate but something else went wrong.
		if (result == false)
		{
			std::stringstream msg;
			msg << "problem reading raw file (" << fileName << ") on open";
			LogError(AudioResult::FILE_ERROR, msg);
			return false;
		}

		// Check for empty files.
		if (fileFrames == 0)
		{
			std::stringstream msg;
			msg << "file is empty upon open (" << fileName << ")";
			LogError(AudioResult::FILE_ERROR, msg);
			return false;
		}

		return true;
	}


	// //////////////////////////////////////////////////////////////////////////////////////////////////

	bool SoundFile::getRawInfo(unsigned int nChannels_, SampleFormat format_, double fileRate_)
	{
		// Use the system call "stat" to determine the file length.
		struct stat filestat;

		if (stat(fileName.c_str(), &filestat) == -1) 
		{
			std::stringstream msg;
			msg << "could not get stats for file (" << fileName << ") during read";
			LogError(AudioResult::FILE_ERROR, msg);

			return false;
		}

		if (nChannels_ == 0) 
		{
			std::stringstream msg;
			msg << "number of channels can't be 0 for file (" << fileName << ") during read";
			LogError(AudioResult::FILE_ERROR, msg);

			return false;
		}

		// Rawwave files have no header and by default, are assumed to
		// contain a monophonic stream of 16-bit signed integers in
		// big-endian byte order at a sample rate of 22050 Hz.  However,
		// different parameters can be specified if desired.

		dataOffset = 0;
		nChannels = nChannels_;
		dataType = format_;
		fileRate = fileRate_;

		int sampleBytes = 0;

		//if (format_ == SampleFormat::SINT8)
		//{
		//	sampleBytes = 1;
		//}
		if (format_ == SampleFormat::SINT16)
		{
			sampleBytes = 2;
		}
		else if (format_ == SampleFormat::SINT32 || format_ == SampleFormat::FLOAT32)
		{
			sampleBytes = 4;
		}
		else if (format_ == SampleFormat::FLOAT64)
		{
			sampleBytes = 8;
		}
		else 
		{
			std::stringstream msg;
			msg << "format " << to_string(format_) << " is invalid in file (" << fileName << ")";
			LogError(AudioResult::FILE_ERROR, msg);
			return false;
		}

		fileFrames = (long)filestat.st_size / sampleBytes / nChannels;  // length in frames

#ifdef __LITTLE_ENDIAN__
		byteswap = true;
#else
		byteswap = false;
#endif
		return true;
	}

	bool SoundFile::getWavInfo()
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
			swap32(&chunkSize);
#endif
			if (fseek(fd, chunkSize, SEEK_CUR) == -1) goto error;
			if (fread(&id, 4, 1, fd) != 1) goto error;
		}

		// Check that the data is not compressed.

		unsigned short format_tag;
		if (fread(&chunkSize, 4, 1, fd) != 1) goto error; // Read fmt chunk size.
		if (fread(&format_tag, 2, 1, fd) != 1) goto error;

#ifndef __LITTLE_ENDIAN__
		swap16(&format_tag);
		swap32(&chunkSize);
#endif
		if (format_tag == 0xFFFE) 
		{ 
			// WAVE_FORMAT_EXTENSIBLE

			dataOffset = ftell(fd);

			if (fseek(fd, 14, SEEK_CUR) == -1) goto error;

			unsigned short extSize;
			if (fread(&extSize, 2, 1, fd) != 1) goto error;

#ifndef __LITTLE_ENDIAN__
			swap16(&extSize);
#endif
			if (extSize == 0) goto error;
			if (fseek(fd, 6, SEEK_CUR) == -1) goto error;

			if (fread(&format_tag, 2, 1, fd) != 1) goto error;

#ifndef __LITTLE_ENDIAN__
			swap16(&format_tag);
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
		swap16(&temp);
#endif
		nChannels = static_cast<unsigned>(temp);

		// Get file sample rate from the header.

		int32_t srate;
		if (fread(&srate, 4, 1, fd) != 1) goto error;

#ifndef __LITTLE_ENDIAN__
		swap32(&srate);
#endif
		fileRate = (double)srate;

		// Determine the data type.

		dataType = SampleFormat::SINT16; // Just a default to shut up compiler

		if (fseek(fd, 6, SEEK_CUR) == -1) goto error;   // Locate bits_per_sample info.
		if (fread(&temp, 2, 1, fd) != 1) goto error;

#ifndef __LITTLE_ENDIAN__
		swap16(&temp);
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
			swap32(&chunkSize);
#endif
			chunkSize += chunkSize % 2; // chunk sizes must be even
			if (fseek(fd, chunkSize, SEEK_CUR) == -1) goto error;
			if (fread(&id, 4, 1, fd) != 1) goto error;
		}

		// Get length of data from the header.
		int32_t bytes;

		if (fread(&bytes, 4, 1, fd) != 1) goto error;

#ifndef __LITTLE_ENDIAN__
		swap32(&bytes);
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

	bool SoundFile::getSndInfo()
	{
		// Determine the data type.
		uint32_t format;
		if (fseek(fd, 12, SEEK_SET) == -1) goto error;   // Locate format
		if (fread(&format, 4, 1, fd) != 1) goto error;

#ifdef __LITTLE_ENDIAN__
		swap32(&format);
#endif

		//if (format == 2) dataType = SampleFormat::SINT8;
		if (format == 3) dataType = SampleFormat::SINT16;
		else if (format == 4) dataType = SampleFormat::SINT24;
		else if (format == 5) dataType = SampleFormat::SINT32;
		else if (format == 6) dataType = SampleFormat::FLOAT32;
		else if (format == 7) dataType = SampleFormat::FLOAT64;
		else
		{
			std::stringstream msg;
			msg << "data format in file " << fileName << " is not supported.";
			LogError(AudioResult::FILE_ERROR, msg);
			return false;
		}

		// Get file sample rate from the header.
		uint32_t srate;
		if (fread(&srate, 4, 1, fd) != 1) goto error;

#ifdef __LITTLE_ENDIAN__
		swap32(&srate);
#endif
		fileRate = (double)srate;

		// Get number of channels from the header.

		uint32_t chans;
		if (fread(&chans, 4, 1, fd) != 1) goto error;

#ifdef __LITTLE_ENDIAN__
		swap32(&chans);
#endif
		nChannels = chans;

		uint32_t offset;
		if (fseek(fd, 4, SEEK_SET) == -1) goto error;
		if (fread(&offset, 4, 1, fd) != 1) goto error;

#ifdef __LITTLE_ENDIAN__
		swap32(&offset);
#endif
		dataOffset = offset;

		// Get length of data from the header.

		unsigned fileSize;
		if (fread(&fileSize, 4, 1, fd) != 1) goto error;

#ifdef __LITTLE_ENDIAN__
		swap32(&fileSize);
#endif
		// Convert to sample frames.

		//if (dataType == SampleFormat::SINT8)
		//{
		//	fileFrames = fileSize / nChannels;
		//}
		if (dataType == SampleFormat::SINT16)
		{
			fileFrames = fileSize / (2 * nChannels);
		}
		else if (dataType == SampleFormat::SINT24)
		{
			fileFrames = fileSize / (3 * nChannels);
		}
		else if (dataType == SampleFormat::SINT32 || dataType == SampleFormat::FLOAT32)
		{
			fileFrames = fileSize / (4 * nChannels);
		}
		else if (dataType == SampleFormat::FLOAT64)
		{
			fileFrames = fileSize / (8 * nChannels);
		}

#ifdef __LITTLE_ENDIAN__
		byteswap = true;
#else
		byteswap = false;
#endif

		return true;

	error:

		std::stringstream msg;
		msg << "reading SND file (" << fileName << ")";
		LogError(AudioResult::FILE_ERROR, msg);
		return false;
	}

	bool SoundFile::getAifInfo()
	{
		bool good_tag = false; // Used later

		bool aifc = false;
		char id[4];

		// Determine whether this is AIFF or AIFC.
		if (fseek(fd, 8, SEEK_SET) == -1) goto error;
		if (fread(&id, 4, 1, fd) != 1) goto error;

		if (!memcmp(id, "AIFC", 4)) aifc = true; // @@ was strncmp, but to shut up compiler ...

		// Find "common" chunk
		int32_t chunkSize;
		if (fread(&id, 4, 1, fd) != 1) goto error;

		while (memcmp(id, "COMM", 4)) // @@ was strncmp, but to shut up compiler ...
		{
			if (fread(&chunkSize, 4, 1, fd) != 1) goto error;

#ifdef __LITTLE_ENDIAN__
			swap32(&chunkSize);
#endif
			chunkSize += chunkSize % 2; // chunk sizes must be even
			if (fseek(fd, chunkSize, SEEK_CUR) == -1) goto error;
			if (fread(&id, 4, 1, fd) != 1) goto error;
		}

		// Get number of channels from the header.
		int16_t temp;
		if (fseek(fd, 4, SEEK_CUR) == -1) goto error; // Jump over chunk size
		if (fread(&temp, 2, 1, fd) != 1) goto error;
		
#ifdef __LITTLE_ENDIAN__
		swap16(&temp);
#endif
		nChannels = temp;

		// Get length of data from the header.
		int32_t frames;
		if (fread(&frames, 4, 1, fd) != 1) goto error;

#ifdef __LITTLE_ENDIAN__
		swap32(&frames);
#endif
		fileFrames = frames; // sample frames

		// Read the number of bits per sample.
		if (fread(&temp, 2, 1, fd) != 1) goto error;

#ifdef __LITTLE_ENDIAN__
		swap16(&temp);
#endif

		// Get file sample rate from the header.  For AIFF files, this value
		// is stored in a 10-byte, IEEE Standard 754 floating point number,
		// so we need to convert it first.
		unsigned char srate[10];
		unsigned char exp;
		unsigned long mantissa;
		unsigned long last;

		if (fread(&srate, 10, 1, fd) != 1) goto error;

		mantissa = (unsigned long)*(unsigned long*)(srate + 2);

#ifdef __LITTLE_ENDIAN__
		swap32(&mantissa);
#endif
		exp = 30 - *(srate + 1);
		last = 0;

		while (exp--) 
		{
			last = mantissa;
			mantissa >>= 1;
		}

		if (last & 0x00000001) mantissa++;

		fileRate = (double)mantissa;

#ifdef __LITTLE_ENDIAN__
		byteswap = true;
#else
		byteswap = false;
#endif

		// Determine the data format.

		dataType = SampleFormat::SINT16; // Just to shut up compiler

		if (aifc == false) 
		{
			//if (temp <= 8)
			//{
			//	dataType = SampleFormat::SINT8;
			//	good_tag = true;
			//}
			if (temp <= 16)
			{
				dataType = SampleFormat::SINT16;
				good_tag = true;
			}
			else if (temp <= 24)
			{
				dataType = SampleFormat::SINT24;
				good_tag = true;
			}
			else if (temp <= 32)
			{
				dataType = SampleFormat::SINT32;
				good_tag = true;
			}
		}
		else 
		{
			if (fread(&id, 4, 1, fd) != 1) goto error;

			if (!memcmp(id, "sowt", 4)) // @@ was strncmp, but to shut up compiler ...
			{ 
				// uncompressed little-endian

				if (byteswap == false) byteswap = true;
				else byteswap = false;
			}

			if (!memcmp(id, "NONE", 4) || !memcmp(id, "sowt", 4)) // @@ was strncmp, but to shut up compiler ...
			{
				//if (temp <= 8)
				//{
				//	dataType = SampleFormat::SINT8;
				//	good_tag = true;
				//}
				if (temp <= 16)
				{
					dataType = SampleFormat::SINT16;
					good_tag = true;
				}
				else if (temp <= 24)
				{
					dataType = SampleFormat::SINT24;
					good_tag = true;
				}
				else if (temp <= 32)
				{
					dataType = SampleFormat::SINT32;
					good_tag = true;
				}
			}
			else if ((!memcmp(id, "fl32", 4) || !memcmp(id, "FL32", 4)) && temp == 32) // @@ was strncmp, but to shut up compiler ...
			{
				dataType = SampleFormat::FLOAT32;
				good_tag = true;
			}
			else if ((!memcmp(id, "fl64", 4) || !memcmp(id, "FL64", 4)) && temp == 64) // @@ was strncmp, but to shut up compiler ...
			{
				dataType = SampleFormat::FLOAT64;
				good_tag = true;
			}
		}

		if (!good_tag) 
		{
			std::stringstream msg;
			msg << "AIFF/AIFC file (" << fileName << ") has unsupported data type (" << id << ").";
			LogError(AudioResult::FILE_ERROR, msg);
			return false;
		}

		// Start at top to find data (SSND) chunk ... chunk order is undefined.
		if (fseek(fd, 12, SEEK_SET) == -1) goto error;

		// Find data (SSND) chunk
		if (fread(&id, 4, 1, fd) != 1) goto error;

		while (memcmp(id, "SSND", 4)) // @@ was strncmp, but to shut up compiler ...
		{
			if (fread(&chunkSize, 4, 1, fd) != 1) goto error;

#ifdef __LITTLE_ENDIAN__
			swap32(&chunkSize);
#endif
			chunkSize += chunkSize % 2; // chunk sizes must be even
			if (fseek(fd, chunkSize, SEEK_CUR) == -1) goto error;
			if (fread(&id, 4, 1, fd) != 1) goto error;
		}

		// Skip over chunk size, offset, and blocksize fields
		if (fseek(fd, 12, SEEK_CUR) == -1) goto error;

		dataOffset = ftell(fd);
		return true;

	error:

		std::stringstream msg;
		msg << "problem reading AIFF file (" << fileName << ").";
		LogError(AudioResult::FILE_ERROR, msg);
		return false;
	}

	bool SoundFile::getMatInfo()
	{
		std::stringstream msg;
		msg << "Mat sound files are unsuported (" << fileName << ").";
		LogError(AudioResult::FILE_ERROR, msg);
		return false;
	}

	// ///////////////////////////////////////////////
	// Finally, the main event
	// ///////////////////////////////////////////////


	bool SoundFile::Read(FrameBuffer<double>& buffer, unsigned startFrame, bool doNormalize)
	{
		// Make sure we have an open file.
		if (fd == 0) 
		{
			std::stringstream msg;
			msg << "file not open (" << fileName << ").";
			LogError(AudioResult::FILE_ERROR, msg);
			return false;
		}

		// Check the buffer size.
		unsigned nFrames = buffer.nFrames;
		if (nFrames == 0) 
		{
			std::stringstream msg;
			msg << "buffer size is zero in (" << fileName << ").";
			LogError(AudioResult::FILE_ERROR, msg);
			return false;
		}

		if (buffer.nChannels != nChannels) 
		{
			//oStream_ << "FileRead::read: StkFrames argument has incompatible number of channels!";
			//Stk::handleError(StkError::FUNCTION_ARGUMENT);

			std::stringstream msg;
			msg << "frame buffer has incompatible number of channels";
			LogError(AudioResult::FUNCTION_ARGUMENT, msg);
			return false;
		}

		if (startFrame >= fileFrames) 
		{
			std::stringstream msg;
			msg << "startFrame argument is >= file size";
			LogError(AudioResult::FUNCTION_ARGUMENT, msg);
			return false;
		}

		// Check for file end.
		if (startFrame + nFrames > fileFrames)
		{
			nFrames = fileFrames - startFrame;
		}

		long i;
		long nSamples = (long)(nFrames * nChannels);
		unsigned long offset = startFrame * nChannels;

		// There are aliasing tricks going on here. dest buffer sample type is actually double
		// but read buffer type might be something else, (but must be <= in size)

		double* dest_buffer = buffer.samples.get();

		// Read samples into StkFrames data buffer.
		if (dataType == SampleFormat::SINT16) 
		{
			auto read_buf = reinterpret_cast<int16_t *>(dest_buffer); // Aliasing!
			if (fseek(fd, dataOffset + (offset * sizeof(int16_t)), SEEK_SET) == -1) goto error;

			if (fread(read_buf, sizeof(int16_t), nSamples, fd) != nSamples) goto error;

			if (byteswap) 
			{
				//endian_swap_16(read_buf, nSamples);
				byteSwapBuffer(dataType, read_buf, nSamples);
			}
			if (doNormalize) 
			{
				double gain = 1.0 / 32768.0;

				// There are aliasing / spacing tricks going on here
				for (i = nSamples - 1; i >= 0; i--)
				{
					dest_buffer[i] = read_buf[i] * gain;
				}
			}
			else 
			{
				// There are aliasing / spacing tricks going on here
				for (i = nSamples - 1; i >= 0; i--)
				{
					dest_buffer[i] = read_buf[i];
				}
			}
		}
		else if (dataType == SampleFormat::SINT32) 
		{
			auto read_buf = reinterpret_cast<int32_t*>(dest_buffer); // Aliasing!
			if (fseek(fd, dataOffset + (offset * sizeof(int32_t)), SEEK_SET) == -1) goto error;

			if (fread(read_buf, sizeof(int32_t), nSamples, fd) != nSamples) goto error;

			if (byteswap) 
			{
				//endian_swap_32(read_buf, nSamples);
				byteSwapBuffer(dataType, read_buf, nSamples);
			}

			if (doNormalize) 
			{
				double gain = 1.0 / 2147483648.0;

				// There are aliasing / spacing tricks going on here
				for (i = nSamples - 1; i >= 0; i--)
				{
					dest_buffer[i] = read_buf[i] * gain;
				}
			}
			else 
			{

				// There are aliasing / spacing tricks going on here
				for (i = nSamples - 1; i >= 0; i--)
				{
					dest_buffer[i] = read_buf[i];
				}
			}
		}
		else if (dataType == SampleFormat::FLOAT32) 
		{
			auto read_buf = reinterpret_cast<float*>(dest_buffer); // Aliasing!
			if (fseek(fd, dataOffset + (offset * sizeof(float)), SEEK_SET) == -1) goto error;

			if (fread(read_buf, sizeof(float), nSamples, fd) != nSamples) goto error;

			if (byteswap) 
			{
				//endian_swap_32(read_buf, nSamples);
				byteSwapBuffer(dataType, read_buf, nSamples);
			}

			// There are aliasing / spacing tricks going on here
			for (i = nSamples - 1; i >= 0; i--)
			{
				dest_buffer[i] = read_buf[i];
			}
		}
		else if (dataType == SampleFormat::FLOAT64) 
		{
			auto read_buf = reinterpret_cast<double *>(dest_buffer); // Aliasing!
			if (fseek(fd, dataOffset + (offset * sizeof(double)), SEEK_SET) == -1) goto error;

			if (fread(read_buf, sizeof(double), nSamples, fd) != nSamples) goto error;

			if (byteswap) 
			{
			    //endian_swap_64(read_buf, nSamples);
				byteSwapBuffer(dataType, read_buf, nSamples);
			}

#if 0
			// Umm, no reason to do this. They point to the same
			// place and have same types.
			for (i = nSamples - 1; i >= 0; i--)
			{
				dest_buffer[i] = read_buf[i];
			}
#endif
		}
#if 0
		else if (dataType == SampleFormat::SINT8 && isWaveFile) 
		{ 
			// 8-bit WAV data is unsigned!
			auto read_buf = reinterpret_cast<unsigned char*>(dest_buffer); // Aliasing!
			if (fseek(fd, dataOffset + offset, SEEK_SET) == -1) goto error;
			if (fread(read_buf, sizeof(unsigned char), nSamples, fd) != nSamples) goto error;

			if (doNormalize) 
			{
				double gain = 1.0 / 128.0;

				// There are aliasing / spacing tricks going on here
				for (i = nSamples - 1; i >= 0; i--)
				{
					dest_buffer[i] = (read_buf[i] - 128) * gain; // convert to signed values
				}
			}
			else 
			{
				// There are aliasing / spacing tricks going on here
				for (i = nSamples - 1; i >= 0; i--)
				{
					dest_buffer[i] = read_buf[i] - 128.0; // convert to signed values
				}
			}
		}
		else if (dataType == SampleFormat::SINT8) 
		{ 
			// signed 8-bit data
			auto read_buf = reinterpret_cast<char*>(dest_buffer); // Aliasing!
			if (fseek(fd, dataOffset + offset, SEEK_SET) == -1) goto error;
			if (fread(read_buf, sizeof(char), nSamples, fd) != nSamples) goto error;

			if (doNormalize) 
			{
				double gain = 1.0 / 128.0;

				// There are aliasing / spacing tricks going on here
				for (i = nSamples - 1; i >= 0; i--)
				{
					dest_buffer[i] = read_buf[i] * gain;
				}
			}
			else 
			{
				// There are aliasing / spacing tricks going on here
				for (i = nSamples - 1; i >= 0; i--)
				{
					dest_buffer[i] = read_buf[i];
				}
			}
		}
#endif
		else if (dataType == SampleFormat::SINT24) 
		{
			// @@ TODO: This needs to be rewritten. Go ahead and use two buffers,
			// c'est la vie! The below is very inefficient. This is important,
			// since I might use 24 bit files a lot.

			// 24-bit values are harder to import efficiently since there is
			// no native 24-bit type.  The following routine works but is much
			// less efficient than that used for the other data types.

			int32_t temp;
			unsigned char* ptr = (unsigned char*)&temp;
			double gain = 1.0 / 2147483648.0;
			if (fseek(fd, dataOffset + (offset * 3), SEEK_SET) == -1) goto error;

			for (i = 0; i < nSamples; i++) {

#ifdef __LITTLE_ENDIAN__
				if (byteswap) 
				{
					if (fread(ptr, sizeof(unsigned char), 3, fd) != 3) goto error;
					temp &= 0x00ffffff;
					swap32(ptr);
				}
				else 
				{
					auto rv = fread(ptr + 1, sizeof(unsigned char), 3, fd);
					if (rv != 3) goto error;
					temp &= 0xffffff00;
				}
#else
				if (byteswap) 
				{
					if (fread(ptr + 1, sizeof(unsigned char), 3, fd) != 3) goto error;
					temp &= 0xffffff00;
					swap32(ptr);
				}
				else 
				{
					if (fread(ptr, sizeof(unsigned char), 3, fd) != 3) goto error;
					temp &= 0x00ffffff;
				}
#endif

				if (doNormalize) 
				{
					dest_buffer[i] = (double)temp * gain; // "gain" also  includes 1 / 256 factor.
				}
				else
				{
					dest_buffer[i] = (double)temp / 256;  // right shift without affecting the sign bit
				}
			}
		}

		buffer.SetDataRate(fileRate);

		return true;

	error:

		std::stringstream msg;
		msg << "unspecified problem reading file (" << fileName << ")";
		LogError(AudioResult::FILE_ERROR, msg);
		return false;
	}

} // end of namespace