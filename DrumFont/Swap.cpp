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
#include <cstdint>
#include "Swap.h"

namespace dfx
{
	void endian_swap_16(void* samples, int n)
	{
		auto s = reinterpret_cast<int16_t*>(samples);

		for (int i = 0; i < n; i++)
		{
			swap16(s++);
		}
	}

	void endian_swap_24(void* samples, int n)
	{
		auto s = reinterpret_cast<int32_t*>(samples);

		for (int i = 0; i < n; i++)
		{
			swap32(s);
			// @@ TODO: Clear out the unused byte?
			++s;
		}
	}

	void endian_swap_32(void* samples, int n)
	{
		auto s = reinterpret_cast<int32_t*>(samples);

		for (int i = 0; i < n; i++)
		{
			swap32(s++);
		}
	}
	
	void endian_swap_64(void* samples, int n)
	{
		auto s = reinterpret_cast<int64_t*>(samples);

		for (int i = 0; i < n; i++)
		{
			swap64(s++);
		}
	}

} // end of namespace