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

#include <cstdint>
#include <string>

namespace dfx
{
#pragma pack(push, 1)

    // LITTE_ENDIAN

    class int24_t {
    protected:

        unsigned char c[3];

    public:

        int24_t() {}

        void operator=(const int32_t& i)
        {
#if 0
            // @@TODO: The data is assummed to be in the lower three bytes
            // of the int32_t val. We might want to assume it's in the
            // upper three bytes.
            c[0] = (i & 0x000000ff);
            c[1] = (i & 0x0000ff00) >> 8;
            c[2] = (i & 0x00ff0000) >> 16;
#else
            // The data is assumed to be in the upper three bytes of
            // the int32_t val. (The other way to think of this is that
            // we are chopping off the lowest byte.) In some circumstances,
            // that lowest byte can be thought of as "dither" we're going
            //to throw away.

            c[0] = (i & 0x000000ff >> 8);
            c[1] = (i & 0x0000ff00) >> 16;
            c[2] = (i & 0x00ff0000) >> 24;
#endif
        }

        int32_t asInt()
        {
#if 0
            // @@TODO: The data is assummed to be in the lower three bytes
            // of the int32_t val. We might want to assume it's in the
            // upper three bytes.
            int32_t i = c[0] | (c[1] << 8) | (c[2] << 16);
            if (i & 0x00800000)
            {
                i |= 0xff000000;
            }
#else
            // For the int32_t return value, we put the 24 bit data in the
            // upper three bytes. The lowest byte is set to zero. That lowest
            // byte is where dither could go if we wanted.
            int32_t i = (c[0] << 8) | (c[1] << 16) | (c[2] << 24);
            return i;
#endif
            return i;
        }

        int24_t *ByteSwap()
        {
            unsigned char t = c[0];
            c[0] = c[3];
            c[3] = t;
            return this;
        }
    };

#pragma pack(pop)

    // ////////////////////////////////////////////

    enum class SampleFormat : long
    {
        // I don't care about this: SINT8,   // -128 to +127
        SINT16,  // -32768 to +32767
        SINT24,  // -8388608 to +8388607
        SINT32,  // -2147483648 to +2147483647.
        FLOAT32, // Normalized between plus/minus 1.0.
        FLOAT64  // Normalized between plus/minus 1.0.
    };

    extern std::string to_string(SampleFormat f);
    extern unsigned nBytes(SampleFormat f);
    extern std::pair<double, double> maxVal(SampleFormat f);

    // ////////////////////////////////////////////

    inline void swap16(void* ptr)
    {
        unsigned char* p = reinterpret_cast<unsigned char*>(ptr);
        unsigned char val;

        // Swap 1st and 2nd bytes
        val = *(p);
        *(p) = *(p + 1);
        *(p + 1) = val;
    }

    inline void swap24(void* ptr)
    {
        unsigned char* p = reinterpret_cast<unsigned char*>(ptr);
        unsigned char val;

        // Swap 1st and 3rd bytes.
        val = *(p);
        *(p) = *(p + 2);
        *(p + 2) = val;
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


    // ////////////////////////////////////////////


    extern void byteSwapBuffer(SampleFormat format, void* buffer, unsigned int nSamples);

    extern void InterleaveChannel(SampleFormat sFormat, void* interleavedBuff, void* nonInterleavedChannel, unsigned whichChannel, unsigned nChannels, unsigned nSamples, bool byte_swap = false);
    extern void DeInterleaveChannel(SampleFormat sFormat, void* interleavedBuff, void* nonInterleavedChannel, unsigned whichChannel, unsigned nChannels, unsigned nSamples, bool byte_swap = false);

    extern void CopyByteSwap(int16_t* dest, int16_t* src, unsigned nSamples, unsigned dest_stride, unsigned src_stride);
    extern void CopyByteSwap(int24_t* dest, int24_t* src, unsigned nSamples, unsigned dest_stride, unsigned src_stride);
    extern void CopyByteSwap(int32_t* dest, int32_t* src, unsigned nSamples, unsigned dest_stride, unsigned src_stride);
    extern void CopyByteSwap(float* dest, float* src, unsigned nSamples, unsigned dest_stride, unsigned src_stride);
    extern void CopyByteSwap(double* dest, double* src, unsigned nSamples, unsigned dest_stride, unsigned src_stride);

    extern void CopyByteSwap(SampleFormat sFormat, void* dest, void* src, unsigned nSamples, unsigned dest_stride, unsigned src_stride);

} // end of namespace
