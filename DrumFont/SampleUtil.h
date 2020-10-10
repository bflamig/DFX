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

#include <string>

namespace dfx
{
	enum class SampleFormat : unsigned long
	{
		SINT8,   // -128 to +127
		SINT16,  // -32768 to +32767
		SINT24,  // Lower 3 bytes of 32-bit signed integer.
		SINT32,  // -2147483648 to +2147483647.
		FLOAT32, // Normalized between plus/minus 1.0.
		FLOAT64  // Normalized between plus/minus 1.0.
	};

	extern std::string to_string(SampleFormat& x);

	inline void swap16(void* ptr)
	{
		unsigned char* p = reinterpret_cast<unsigned char*>(ptr);
		unsigned char val;

		// Swap 1st and 2nd bytes
		val = *(p);
		*(p) = *(p + 1);
		*(p + 1) = val;
	}

	inline void swap32(void* ptr)
	{
		unsigned char* p = reinterpret_cast<unsigned char*>(ptr);
		unsigned char val;

		// Swap 1st and 4th bytes
		val = *(p);
		*(p) = *(p + 3);
		*(p + 3) = val;

		//Swap 2nd and 3rd bytes
		p += 1;
		val = *(p);
		*(p) = *(p + 1);
		*(p + 1) = val;
	}

	inline void swap64(void* ptr)
	{
		unsigned char* p = reinterpret_cast<unsigned char*>(ptr);
		unsigned char val;

		// Swap 1st and 8th bytes
		val = *(p);
		*(p) = *(p + 7);
		*(p + 7) = val;

		// Swap 2nd and 7th bytes
		p += 1;
		val = *(p);
		*(p) = *(p + 5);
		*(p + 5) = val;

		// Swap 3rd and 6th bytes
		p += 1;
		val = *(p);
		*(p) = *(p + 3);
		*(p + 3) = val;

		// Swap 4th and 5th bytes
		p += 1;
		val = *(p);
		*(p) = *(p + 1);
		*(p + 1) = val;
	}

	void endian_swap_16(void* samples, int n);
	void endian_swap_24(void* samples, int n);
	void endian_swap_32(void* samples, int n);
	void endian_swap_64(void* samples, int n);

#if 0

	inline void conditional_swap16(void* samples, int n, bool samples_are_little_endian)
	{
#ifdef __LITTLE_ENDIAN__
		if (!samples_are_little_endian)
		{
			endian_swap_16(samples, n);
		}
#else   // __BIG_ENDIAN__
		if (samples_are_little_endian)
		{
			endian_swap_16(samples, n);
		}
#endif
	}

	inline void conditional_swap24(void* samples, int n, bool samples_are_little_endian)
	{
#ifdef __LITTLE_ENDIAN__
		if (!samples_are_little_endian)
		{
			endian_swap_24(samples, n);
		}
#else	// __BIG_ENDIAN__
		if (samples_are_little_endian)
		{
			endian_swap_24(samples, n);
		}
#endif
	}

	inline void conditional_swap32(void* samples, int n, bool samples_are_little_endian)
	{
#ifdef __LITTLE_ENDIAN__
		if (!samples_are_little_endian)
		{
			endian_swap_32(samples, n);
		}
#else	// __BIG_ENDIAN__
		if (samples_are_little_endian)
		{
			endian_swap_32(samples, n);
		}
#endif
	}

	inline void conditional_swap64(void* samples, int n, bool samples_are_little_endian)
	{
#ifdef __LITTLE_ENDIAN__
		if (!samples_are_little_endian)
		{
			endian_swap_64(samples, n);
		}
#else	// __BIG_ENDIAN__
		if (samples_are_little_endian)
		{
			endian_swap_64(samples, n);
		}
#endif
	}

#endif

};
