#include "SampleUtil.h"
#include <cstring>

namespace dfx
{
	std::string to_string(SampleFormat& x)
	{
		switch (x)
		{
			case SampleFormat::SINT16: return "SINT16";
			case SampleFormat::SINT24: return "SINT24";
			case SampleFormat::SINT32: return "SINT32";
			case SampleFormat::FLOAT32: return "FLOAT32";
			case SampleFormat::FLOAT64: return "FLOAT64";
			default:
			return "Unknown Sample Format";
		}
	}


    unsigned nBytes(SampleFormat f)
    {
        switch (f)
        {
            case SampleFormat::SINT16: return 2;
            case SampleFormat::SINT24: return 3;
            case SampleFormat::SINT32: return 4;
            case SampleFormat::FLOAT32: return 4;
            case SampleFormat::FLOAT64: return 8;
            default: return 0;
        }
    }


    //static inline uint16_t bswap_16(uint16_t x) { return (x>>8) | (x<<8); }
    //static inline uint32_t bswap_32(uint32_t x) { return (bswap_16(x&0xffff)<<16) | (bswap_16(x>>16)); }
    //static inline uint64_t bswap_64(uint64_t x) { return (((unsigned long long)bswap_32(x&0xffffffffull))<<32) | (bswap_32(x>>32)); }

    void byteSwapBuffer(SampleFormat format, void* buffer, unsigned int nSamples)
    {
        unsigned char val;
        unsigned char* ptr;

        ptr = reinterpret_cast<unsigned char*>(buffer);

        switch (format)
        {
            case SampleFormat::SINT16:
            {
                for (unsigned int i = 0; i < nSamples; i++)
                {
                    // Swap 1st and 2nd bytes.
                    val = *(ptr);
                    *(ptr) = *(ptr + 1);
                    *(ptr + 1) = val;

                    // Increment 2 bytes.
                    ptr += 2;
                }
            }
            break;

            case SampleFormat::SINT32:
            case SampleFormat::FLOAT32:
            {
                for (unsigned int i = 0; i < nSamples; i++)
                {
                    // Swap 1st and 4th bytes.
                    val = *(ptr);
                    *(ptr) = *(ptr + 3);
                    *(ptr + 3) = val;

                    // Swap 2nd and 3rd bytes.
                    ptr += 1;
                    val = *(ptr);
                    *(ptr) = *(ptr + 1);
                    *(ptr + 1) = val;

                    // Increment 3 more bytes.
                    ptr += 3;
                }
            }
            break;

            case SampleFormat::SINT24:
            {
                for (unsigned int i = 0; i < nSamples; i++)
                {
                    // Swap 1st and 3rd bytes.
                    val = *(ptr);
                    *(ptr) = *(ptr + 2);
                    *(ptr + 2) = val;

                    // Increment 2 more bytes.
                    ptr += 2;
                }
            }
            break;

            case SampleFormat::FLOAT64:
            {
                for (unsigned int i = 0; i < nSamples; i++)
                {
                    // Swap 1st and 8th bytes
                    val = *(ptr);
                    *(ptr) = *(ptr + 7);
                    *(ptr + 7) = val;

                    // Swap 2nd and 7th bytes
                    ptr += 1;
                    val = *(ptr);
                    *(ptr) = *(ptr + 5);
                    *(ptr + 5) = val;

                    // Swap 3rd and 6th bytes
                    ptr += 1;
                    val = *(ptr);
                    *(ptr) = *(ptr + 3);
                    *(ptr + 3) = val;

                    // Swap 4th and 5th bytes
                    ptr += 1;
                    val = *(ptr);
                    *(ptr) = *(ptr + 1);
                    *(ptr + 1) = val;

                    // Increment 5 more bytes.
                    ptr += 5;
                }
            }
            break;
        }
    }

	// ////////////////////////////////////////

	void CopyByteSwap(int16_t* dest, int16_t *src, unsigned nSamples, unsigned dest_stride, unsigned src_stride)
	{
		int16_t* pd = dest;
		int16_t* ps = src;
		int16_t buff;

		for (unsigned i = 0; i < nSamples; i++)
		{
			memcpy(&buff, ps, sizeof(int16_t));

			unsigned char* ptr = reinterpret_cast<unsigned char *>(&buff);
			unsigned char val;

			// Swap 1st and 2nd bytes.
			val = *(ptr);
			*(ptr) = *(ptr + 1);
			*(ptr + 1) = val;

			memcpy(pd, &buff, sizeof(int16_t));

			pd += dest_stride;
			ps += src_stride;

		}
	}


	void CopyByteSwap(int24_t* dest, int24_t* src, unsigned nSamples, unsigned dest_stride, unsigned src_stride)
	{
		int24_t* pd = dest;
		int24_t* ps = src;
		int24_t buff;

		for (unsigned i = 0; i < nSamples; i++)
		{
			memcpy(&buff, ps, sizeof(int24_t));

			unsigned char* ptr = reinterpret_cast<unsigned char*>(&buff);
			unsigned char val;

			// Swap 1st and 3rd bytes.
			val = *(ptr);
			*(ptr) = *(ptr + 2);
			*(ptr + 2) = val;

			memcpy(pd, &buff, sizeof(int24_t));

			pd += dest_stride;
			ps += src_stride;
		}
	}

	void CopyByteSwap(int32_t* dest, int32_t* src, unsigned nSamples, unsigned dest_stride, unsigned src_stride)
	{
		int32_t* pd = dest;
		int32_t* ps = src;
		int32_t buff;

		for (unsigned i = 0; i < nSamples; i++)
		{
			memcpy(&buff, ps, sizeof(int32_t));

			unsigned char* ptr = reinterpret_cast<unsigned char*>(&buff);
			unsigned char val;

			// Swap 1st and 4th bytes.
			val = *(ptr);
			*(ptr) = *(ptr + 3);
			*(ptr + 3) = val;

			// Swap 2nd and 3rd bytes.
			ptr += 1;
			val = *(ptr);
			*(ptr) = *(ptr + 1);
			*(ptr + 1) = val;

			memcpy(pd, &buff, sizeof(int32_t));

			pd += dest_stride;
			ps += src_stride;
		}
	}

	void CopyByteSwap(float* dest, float* src, unsigned nSamples, unsigned dest_stride, unsigned src_stride)
	{
		float* pd = dest;
		float* ps = src;
		float buff;

		for (unsigned i = 0; i < nSamples; i++)
		{
			memcpy(&buff, ps, sizeof(float));

			unsigned char* ptr = reinterpret_cast<unsigned char*>(&buff);
			unsigned char val;

			// Swap 1st and 4th bytes.
			val = *(ptr);
			*(ptr) = *(ptr + 3);
			*(ptr + 3) = val;

			// Swap 2nd and 3rd bytes.
			ptr += 1;
			val = *(ptr);
			*(ptr) = *(ptr + 1);
			*(ptr + 1) = val;

			memcpy(pd, &buff, sizeof(float));

			pd += dest_stride;
			ps += src_stride;
		}
	}

	void CopyByteSwap(double* dest, double* src, unsigned nSamples, unsigned dest_stride, unsigned src_stride)
	{
		double* pd = dest;
		double* ps = src;
		double buff;

		for (unsigned i = 0; i < nSamples; i++)
		{
			memcpy(&buff, ps, sizeof(double));

			unsigned char* ptr = reinterpret_cast<unsigned char*>(&buff);
			unsigned char val;

			// Swap 1st and 8th bytes
			val = *(ptr);
			*(ptr) = *(ptr + 7);
			*(ptr + 7) = val;

			// Swap 2nd and 7th bytes
			ptr += 1;
			val = *(ptr);
			*(ptr) = *(ptr + 5);
			*(ptr + 5) = val;

			// Swap 3rd and 6th bytes
			ptr += 1;
			val = *(ptr);
			*(ptr) = *(ptr + 3);
			*(ptr + 3) = val;

			// Swap 4th and 5th bytes
			ptr += 1;
			val = *(ptr);
			*(ptr) = *(ptr + 1);
			*(ptr + 1) = val;

			memcpy(pd, &buff, sizeof(double));

			pd += dest_stride;
			ps += src_stride;
		}
	}

	void CopyByteSwap(SampleFormat sFormat, void* dest, void* src, unsigned nSamples, unsigned dest_stride, unsigned src_stride)
	{
		switch (sFormat)
		{
			case SampleFormat::SINT16:
			{
				int16_t* d = reinterpret_cast<int16_t*>(dest);
				int16_t* s = reinterpret_cast<int16_t*>(src);
				CopyByteSwap(d, s, nSamples, dest_stride, src_stride);
			}
			break;

			case SampleFormat::SINT24:
			{
				int24_t* d = reinterpret_cast<int24_t*>(dest);
				int24_t* s = reinterpret_cast<int24_t*>(src);
				CopyByteSwap(d, s, nSamples, dest_stride, src_stride);
			}
			break;

			case SampleFormat::SINT32:
			{
				int32_t* d = reinterpret_cast<int32_t*>(dest);
				int32_t* s = reinterpret_cast<int32_t*>(src);
				CopyByteSwap(d, s, nSamples, dest_stride, src_stride);
			}
			break;

			case SampleFormat::FLOAT32:
			{
				float* d = reinterpret_cast<float*>(dest);
				float* s = reinterpret_cast<float*>(src);
				CopyByteSwap(d, s, nSamples, dest_stride, src_stride);
			}
			break;

			case SampleFormat::FLOAT64:
			{
				double* d = reinterpret_cast<double*>(dest);
				double* s = reinterpret_cast<double*>(src);
				CopyByteSwap(d, s, nSamples, dest_stride, src_stride);
			}
			break;
		}
	}

#if 0

	// ////////////////////////////////////////

	void ByteSwapSrc(int16_t& dest, int16_t src)
	{
		unsigned char* src_ptr = reinterpret_cast<unsigned char*>(&src);
		unsigned char* ptr = src_ptr;
		unsigned char val;

		// Swap 1st and 2nd bytes.
		val = *(ptr);
		*(ptr) = *(ptr + 1);
		*(ptr + 1) = val;

		memcpy(&dest, src_ptr, sizeof(int16_t));
	}


	void ByteSwapDest(int16_t& dest, int16_t src)
	{
		memcpy(&dest, &src, sizeof(int16_t));

		unsigned char* ptr = reinterpret_cast<unsigned char *>(&dest);
		unsigned char val;

		// Swap 1st and 2nd bytes.
		val = *(ptr);
		*(ptr) = *(ptr + 1);
		*(ptr + 1) = val;
	}

	void ByteSwapSrc(int24_t& dest, int24_t src)
	{
		unsigned char* src_ptr = reinterpret_cast<unsigned char *>(&src);
		unsigned char* ptr = src_ptr;
		unsigned char val;

		// Swap 1st and 3rd bytes.
		val = *(ptr);
		*(ptr) = *(ptr + 2);
		*(ptr + 2) = val;

		memcpy(&dest, src_ptr, sizeof(int24_t));
	}

	void ByteSwapDest(int24_t& dest, int24_t src)
	{
		memcpy(&dest, &src, sizeof(int24_t));

		unsigned char* ptr = reinterpret_cast<unsigned char*>(&dest);
		unsigned char val;

		// Swap 1st and 3rd bytes.
		val = *(ptr);
		*(ptr) = *(ptr + 2);
		*(ptr + 2) = val;
	}

	void ByteSwapSrc(int32_t& dest, int32_t src)
	{
		unsigned char* src_ptr = reinterpret_cast<unsigned char*>(&src);
		unsigned char* ptr = src_ptr;
		unsigned char val;

		// Swap 1st and 4th bytes.
		val = *(ptr);
		*(ptr) = *(ptr + 3);
		*(ptr + 3) = val;

		// Swap 2nd and 3rd bytes.
		ptr += 1;
		val = *(ptr);
		*(ptr) = *(ptr + 1);
		*(ptr + 1) = val;

		memcpy(&dest, src_ptr, sizeof(int32_t));
	}


	void ByteSwapDest(int32_t& dest, int32_t src)
	{
		memcpy(&dest, &src, sizeof(int32_t));

		unsigned char* ptr = reinterpret_cast<unsigned char *>(&dest);
		unsigned char val;

		// Swap 1st and 4th bytes.
		val = *(ptr);
		*(ptr) = *(ptr + 3);
		*(ptr + 3) = val;

		// Swap 2nd and 3rd bytes.
		ptr += 1;
		val = *(ptr);
		*(ptr) = *(ptr + 1);
		*(ptr + 1) = val;
	}

	void ByteSwapSrc(float& dest, float src)
	{
		unsigned char* src_ptr = reinterpret_cast<unsigned char*>(&dest);
		unsigned char* ptr = src_ptr;
		unsigned char val;

		// Swap 1st and 4th bytes.
		val = *(ptr);
		*(ptr) = *(ptr + 3);
		*(ptr + 3) = val;

		// Swap 2nd and 3rd bytes.
		ptr += 1;
		val = *(ptr);
		*(ptr) = *(ptr + 1);
		*(ptr + 1) = val;

		memcpy(&dest, src_ptr, sizeof(float));
	}

	void ByteSwapDest(float& dest, float src)
	{
		memcpy(&dest, &src, sizeof(float));

		unsigned char* ptr = reinterpret_cast<unsigned char*>(&dest);
		unsigned char val;

		// Swap 1st and 4th bytes.
		val = *(ptr);
		*(ptr) = *(ptr + 3);
		*(ptr + 3) = val;

		// Swap 2nd and 3rd bytes.
		ptr += 1;
		val = *(ptr);
		*(ptr) = *(ptr + 1);
		*(ptr + 1) = val;
	}


	void ByteSwapSrc(double& dest, double src)
	{
		unsigned char* src_ptr = reinterpret_cast<unsigned char*>(&src);
		unsigned char* ptr = src_ptr;
		unsigned char val;

		// Swap 1st and 8th bytes
		val = *(ptr);
		*(ptr) = *(ptr + 7);
		*(ptr + 7) = val;

		// Swap 2nd and 7th bytes
		ptr += 1;
		val = *(ptr);
		*(ptr) = *(ptr + 5);
		*(ptr + 5) = val;

		// Swap 3rd and 6th bytes
		ptr += 1;
		val = *(ptr);
		*(ptr) = *(ptr + 3);
		*(ptr + 3) = val;

		// Swap 4th and 5th bytes
		ptr += 1;
		val = *(ptr);
		*(ptr) = *(ptr + 1);
		*(ptr + 1) = val;

		memcpy(&dest, src_ptr, sizeof(double));
	}

	void ByteSwapDest(double& dest, double src)
	{
		memcpy(&dest, &src, sizeof(double));

		unsigned char* ptr = reinterpret_cast<unsigned char*>(&dest);
		unsigned char val;

		// Swap 1st and 8th bytes
		val = *(ptr);
		*(ptr) = *(ptr + 7);
		*(ptr + 7) = val;

		// Swap 2nd and 7th bytes
		ptr += 1;
		val = *(ptr);
		*(ptr) = *(ptr + 5);
		*(ptr + 5) = val;

		// Swap 3rd and 6th bytes
		ptr += 1;
		val = *(ptr);
		*(ptr) = *(ptr + 3);
		*(ptr + 3) = val;

		// Swap 4th and 5th bytes
		ptr += 1;
		val = *(ptr);
		*(ptr) = *(ptr + 1);
		*(ptr + 1) = val;
	}

#endif

	// ////////////////////////////////////////////////////////////////////////////////
	//
	// Due to ASIO architecture, can only conveniently do these one channel at a time
	//
	// ////////////////////////////////////////////////////////////////////////////////

	template<typename T>
	void InterleaveChannel(T* interleavedBuff, T* nonInterleavedChannel, unsigned whichChannel, unsigned nChannels, unsigned nSamples)
	{
		auto src = nonInterleavedChannel;
		auto dest = interleavedBuff + whichChannel; // bug! *nChannels;

		for (unsigned s = 0; s < nSamples; s++)
		{
			*dest = *src++;
			dest += nChannels;
		}
	}

	template<typename T>
	void InterleaveChannel(T* interleavedBuff, T* nonInterleavedChannel, unsigned whichChannel, unsigned nChannels, unsigned nSamples, bool byte_swap)
	{
		auto src = nonInterleavedChannel;
		auto dest = interleavedBuff + whichChannel; // bug! *nChannels;

		if (byte_swap)
		{
			CopyByteSwap(dest, src, nSamples, nChannels, 1);
		}
		else
		{
			for (unsigned s = 0; s < nSamples; s++)
			{
				*dest = *src++;
				dest += nChannels;
			}
		}
	}

	template<typename T>
	void DeInterleaveChannel(T* interleavedBuff, T* nonInterleavedChannel, unsigned whichChannel, unsigned nChannels, unsigned nSamples)
	{
		auto dest = nonInterleavedChannel;
		auto src = interleavedBuff + whichChannel; // bug! *nChannels;

		for (unsigned s = 0; s < nSamples; s++)
		{
			*dest++ = *src;
			src += nChannels;
		}
	}

	template<typename T>
	void DeInterleaveChannel(T* interleavedBuff, T* nonInterleavedChannel, unsigned whichChannel, unsigned nChannels, unsigned nSamples, bool byte_swap)
	{
		auto dest = nonInterleavedChannel;
		auto src = interleavedBuff + whichChannel; // bug! *nChannels;

		if (byte_swap)
		{
			CopyByteSwap(dest, src, nSamples, 1, nChannels);
		}
		else
		{
			for (unsigned s = 0; s < nSamples; s++)
			{
				*dest++ = *src;
				src += nChannels;
			}
		}
	}

	void InterleaveChannel(SampleFormat sFormat, void* interleavedBuff, void* nonInterleavedChannel, unsigned whichChannel, unsigned nChannels, unsigned nSamples, bool byte_swap)
	{
		switch (sFormat)
		{
			case SampleFormat::SINT16:  // -32768 to +32767
			{
				auto ix = reinterpret_cast<int16_t*>(interleavedBuff);
				auto nx = reinterpret_cast<int16_t*>(nonInterleavedChannel);
				InterleaveChannel(ix, nx, whichChannel, nChannels, nSamples, byte_swap);
			}
			break;
			case SampleFormat::SINT24:  // -8388608 to +8388607
			{
				auto ix = reinterpret_cast<int24_t*>(interleavedBuff);
				auto nx = reinterpret_cast<int24_t*>(nonInterleavedChannel);
				InterleaveChannel(ix, nx, whichChannel, nChannels, nSamples, byte_swap);
			}
			break;
			case SampleFormat::SINT32:  // -2147483648 to +2147483647.
			{
				auto ix = reinterpret_cast<int32_t*>(interleavedBuff);
				auto nx = reinterpret_cast<int32_t*>(nonInterleavedChannel);
				InterleaveChannel(ix, nx, whichChannel, nChannels, nSamples, byte_swap);
			}
			break;
			case SampleFormat::FLOAT32: // Normalized between plus/minus 1.0.
			{
				auto ix = reinterpret_cast<float*>(interleavedBuff);
				auto nx = reinterpret_cast<float*>(nonInterleavedChannel);
				InterleaveChannel(ix, nx, whichChannel, nChannels, nSamples, byte_swap);
			}
			break;
			case SampleFormat::FLOAT64:  // Normalized between plus/minus 1.0.
			{
				auto ix = reinterpret_cast<double*>(interleavedBuff);
				auto nx = reinterpret_cast<double*>(nonInterleavedChannel);
				InterleaveChannel(ix, nx, whichChannel, nChannels, nSamples, byte_swap);
			}
			break;
		}
	}

	void DeInterleaveChannel(SampleFormat sFormat, void* interleavedBuff, void* nonInterleavedChannel, unsigned whichChannel, unsigned nChannels, unsigned nSamples, bool byte_swap)
	{
		switch (sFormat)
		{
			case SampleFormat::SINT16:  // -32768 to +32767
			{
				auto ix = reinterpret_cast<int16_t*>(interleavedBuff);
				auto nx = reinterpret_cast<int16_t*>(nonInterleavedChannel);
				DeInterleaveChannel(ix, nx, whichChannel, nChannels, nSamples, byte_swap);
			}
			break;
			case SampleFormat::SINT24:  // -8388608 to +8388607
			{
				auto ix = reinterpret_cast<int24_t*>(interleavedBuff);
				auto nx = reinterpret_cast<int24_t*>(nonInterleavedChannel);
				DeInterleaveChannel(ix, nx, whichChannel, nChannels, nSamples, byte_swap);
			}
			break;
			case SampleFormat::SINT32:  // -2147483648 to +2147483647.
			{
				auto ix = reinterpret_cast<int32_t*>(interleavedBuff);
				auto nx = reinterpret_cast<int32_t*>(nonInterleavedChannel);
				DeInterleaveChannel(ix, nx, whichChannel, nChannels, nSamples, byte_swap);
			}
			break;
			case SampleFormat::FLOAT32: // Normalized between plus/minus 1.0.
			{
				auto ix = reinterpret_cast<float*>(interleavedBuff);
				auto nx = reinterpret_cast<float*>(nonInterleavedChannel);
				DeInterleaveChannel(ix, nx, whichChannel, nChannels, nSamples, byte_swap);
			}
			break;
			case SampleFormat::FLOAT64:  // Normalized between plus/minus 1.0.
			{
				auto ix = reinterpret_cast<double*>(interleavedBuff);
				auto nx = reinterpret_cast<double*>(nonInterleavedChannel);
				DeInterleaveChannel(ix, nx, whichChannel, nChannels, nSamples, byte_swap);
			}
			break;
		}
	}

} // end of namespace