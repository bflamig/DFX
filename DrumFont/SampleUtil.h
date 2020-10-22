#pragma once

#include <cstdint>
#include <string>

namespace dfx
{
#pragma pack(push, 1)

    // @@ THIS IS PROBABLY LITTE_ENDIAN ONLY HERE!

    class int24_t {
    protected:

        unsigned char c3[3];

    public:

        int24_t() {}

#if 0

        int24_t(const double& d)
        {
            *this = (int)d;
        }

        int24_t(const float& f)
        {
            *this = (int)f;
        }

        int24_t(const short& s)
        {
            *this = (int)s;
        }

#endif

#if 0
        int24_t(const int& s) // @@ Why not this too?
        {
            *this = (int)s;
        }
#endif


#if 0
        int24_t(const char& c)
        {
            *this = (int)c;
        }

#endif

        void operator=(const int& i)
        {
            c3[0] = (i & 0x000000ff);
            c3[1] = (i & 0x0000ff00) >> 8;
            c3[2] = (i & 0x00ff0000) >> 16;
        }

        int asInt()
        {
            int i = c3[0] | (c3[1] << 8) | (c3[2] << 16);
            if (i & 0x800000) i |= ~0xffffff;
            return i;
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

    extern std::string to_string(SampleFormat& x);
    extern unsigned nBytes(SampleFormat);

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
