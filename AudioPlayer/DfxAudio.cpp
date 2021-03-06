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
 * 
 * NOTICE: This code was inspired by code in the RtAudio.h/RtAudio.cpp files
 * of the Synthesis ToolKit (STK). 
 * However this code is not a replacement for any of the classes represented
 * there. Instead, it just implements the functionality needed for Dfx support,
 * and has been heavily modified and cleaned up using modern C++ (2017) features.
 * Herein, we acknowledge the work that has gone before us:
 * 
 *  RtAudio GitHub site: https://github.com/thestk/rtaudio
 *  RtAudio WWW site: http://www.music.mcgill.ca/~gary/rtaudio/
 *
 *  RtAudio: realtime audio i/o C++ classes
 *  Copyright (c) 2001-2019 Gary P. Scavone
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

#include "DfxAudio.h"

#ifdef __OS_WINDOWS__
#include "AsioMgr.h"
#endif

#ifdef __UNIX_JACK__
#include "JackMgr.h"
#endif

#ifdef __MACOSX_CORE__
#incldue "CoreMgr.h"
#endif

namespace dfx
{

    std::unique_ptr<DfxAudio> MakeAudioApi()
    {
        std::unique_ptr<DfxAudio> dfa;

#ifdef __OS_WINDOWS__
        dfa = std::make_unique<AsioMgr>();
#elif __UNIX_JACK__
        dfa = std::make_unique<JackMgr>();
#elif __MACOSX_CORE__
        dfa = std::make_unique<CoreMgr>();
#endif

        return dfa;
    }


#if 0
    std::unique_ptr<DfxAudio> MakeAudioApi(AudioApi api_)
    {
        std::unique_ptr<DfxAudio> dfa;

        switch (api_)
        {
            case AudioApi::ASIO:
            {
#ifdef __OS_WINDOWS__
                dfa = std::make_unique<AsioMgr>();
#endif
            }
            break;

            case AudioApi::Jack:
            {
#ifdef __UNIX_JACK__
                dfa = std::make_unique<JackMgr>();
#endif
            }
            break;
            case AudioApi::Core:
            {
#ifdef __MACOSX_CORE__
                dfa = std::make_unique<CoreMgr>();
#endif
            }
            default:;
        }

        return dfa;
    }

#endif

    // ////////////////////////////////////////////////////////////////////////
    // 
    // 
    // ////////////////////////////////////////////////////////////////////////

    DfxAudio::DfxAudio()
    : stream{}
    , devInfo{}
    {
    }

    DfxAudio::~DfxAudio()
    {
    }

    bool DfxAudio::Prep(const std::string& driver_name, bool verbose)
    {
        bool b = LoadDriver(driver_name);

        if (b)
        {
            b = InitDriver(verbose);
        }

        return b;
    }

    bool DfxAudio::Start()
    {
        // should be overidden
        stream.state = StreamState::RUNNING;
        return true;
    }

    bool DfxAudio::Stop()
    {
        // should be overidden
        stream.state = StreamState::STOPPED;
        return true;
    }

    void DfxAudio::verifyStream()
    {
        if (stream.state == StreamState::CLOSED)
        {
            //errorText_ = "stream not open";
            //error(RtAudioError::INVALID_USE);
        }
    }

    void DfxAudio::closeStream()
    {
        // derived classes should override here

        delete stream.userPlayBuffer;
        stream.userPlayBuffer = 0;

        delete stream.userRecBuffer;
        stream.userRecBuffer = 0;

        delete stream.devPlayBuffer;
        stream.devPlayBuffer = 0;

        delete stream.devPlayBuffer;
        stream.devPlayBuffer = 0;

        //stream_.mode = UNINITIALIZED;
        stream.state = StreamState::CLOSED;
    }

    void DfxAudio::abortStream()
    {
        verifyStream();

        if (stream.state == StreamState::STOPPED)
        {
            return;
        }

        // The following lines were commented-out because some behavior was
        // noted where the device buffers need to be zeroed to avoid
        // continuing sound, even when the device buffers are completely
        // disposed.  So now, calling abort is the same as calling stop.
        // AsioHandle *handle = (AsioHandle *) stream_.apiHandle;
        // handle->drainCounter = 2;

        stopStream();
    }

    double DfxAudio::getStreamTime()
    {
        verifyStream();

#if defined( HAVE_GETTIMEOFDAY )
        // Return a very accurate estimate of the stream time by
        // adding in the elapsed time since the last tick.
        struct timeval then;
        struct timeval now;

        if (stream_.state != STREAM_RUNNING || stream_.streamTime == 0.0)
            return stream_.streamTime;

        gettimeofday(&now, NULL);
        then = stream_.lastTickTimestamp;
        return stream_.streamTime +
            ((now.tv_sec + 0.000001 * now.tv_usec) -
                (then.tv_sec + 0.000001 * then.tv_usec));
#else
        return stream.streamTime;
#endif
    }

    void DfxAudio::setStreamTime(double time)
    {
        verifyStream();

        if (time >= 0.0)
        {
            stream.streamTime = time;
        }

#if defined( HAVE_GETTIMEOFDAY )
        gettimeofday(&stream_.lastTickTimestamp, NULL);
#endif
    }

    void DfxAudio::tickStreamTime()
    {
        // This is called once per callback event. So we've processed
        // stream.bufferSize (eg 64) samples. So advance the stream time
        // to the start of the next buffer.

        // Subclasses that do not provide their own implementation of
        // getStreamTime should call this function once per buffer I/O to
        // provide basic stream time support.

        stream.streamTime += (double(stream.bufferSize) / double(stream.sampleRate));

#if defined( HAVE_GETTIMEOFDAY )
        gettimeofday(&stream_.lastTickTimestamp, NULL);
#endif
    }

    // ////////////////////////////////////////////////////////////////////////
    // 
    // 
    // ////////////////////////////////////////////////////////////////////////

    DeviceInfo::DeviceInfo()
    : supportedSampleRates{}
    , inputNames{}
    , outputNames{}
    , name{}
    , devID{}
    , nOutChannelsAvail{}
    , nInChannelsAvail{}
    , nDuplexChannelsAvail{}
    , isDefaultOutput{}
    , isDefaultInput{}
    , preferredSampleRate{}
    , format{}
    , little_endian{}
    , valid{}
    {
    }

    // Just getting experience with move semantics.
    // Not necessarily needed.

    DeviceInfo::DeviceInfo(DeviceInfo&& other) noexcept
    : supportedSampleRates{ std::move(other.supportedSampleRates) }
    , inputNames{ std::move(other.inputNames) }
    , outputNames{ std::move(other.outputNames) }
    , name{ std::move(other.name) }
    , devID(other.devID)
    , nOutChannelsAvail{ other.nOutChannelsAvail }
    , nInChannelsAvail{ other.nInChannelsAvail }
    , nDuplexChannelsAvail{ other.nDuplexChannelsAvail }
    , isDefaultOutput{ other.isDefaultOutput }
    , isDefaultInput{ other.isDefaultInput }
    , preferredSampleRate{ other.preferredSampleRate }
    , format{ other.format }
    , little_endian(other.little_endian)
    , valid{ other.valid }
    {
        // Not bothering to clear the non std::move'd fields
    }

    bool DeviceInfo::IsCompatibleChannelRange(IoMode mode, long nChannels, long startChannel)
    {
        if (mode == IoMode::In)
        {
            return (nChannels + startChannel <= nInChannelsAvail);
        }
        else if (mode == IoMode::Out)
        {
            return (nChannels + startChannel <= nOutChannelsAvail);
        }
        else // mode == IoMode::Duplex
        {
            return (nChannels + startChannel <= nDuplexChannelsAvail);
        }
    }

    bool DeviceInfo::IsCompatibleSampleRate(unsigned sampleRate)
    {
        for (auto& s : supportedSampleRates)
        {
            if (s == sampleRate) return true;
        }

        return false;
    }

    bool DeviceInfo::IsCompatibleFormat(SampleFormat fmt)
    {
        //bool b = deviceFormats.test(static_cast<size_t>(fmt));
        //return b;
        return fmt == format;
    }

    // ///////////////////////////////////////////////////////////////////////////
    // 
    // ///////////////////////////////////////////////////////////////////////////


    DfxStream::DfxStream()
    {
        devPlayID = 11111;
        devRecID = 11111;
    }

    DfxStream::~DfxStream()
    {

    }


	bool DfxStream::FinishBufferConfig()
	{
		// All platform independent stream stuff from here on out

		bool b = true;

		doConvertPlayBuffer = (devPlayFormat != userFormat || devPlayInterleaved != userInterleaved);
		doConvertRecBuffer = (devRecFormat != userFormat || devRecInterleaved != userInterleaved);

		// Allocate necessary internal buffers

		if (nUserRecChannels > 0)
		{
			auto nb = bufferSize;
			nb *= nUserRecChannels;
			nb *= nBytes(userFormat);

			userRecBuffer = new char[nb];

			if (userRecBuffer == 0)
			{
				// @@ TODO: error message
				b = false;
			}

			if (doConvertRecBuffer)
			{
				nb = bufferSize;
				nb *= nDevRecChannels;
				nb *= nBytes(devRecFormat);
				devRecBuffer = new char[nb];

				if (devRecBuffer == 0)
				{
					// @@ TODO: error message
					b = false;
				}
			}

		}

		if (nUserPlayChannels > 0)
		{
			auto nb = bufferSize;
			nb *= nUserPlayChannels;
			nb *= nBytes(userFormat);

			userPlayBuffer = new char[nb];

			if (userPlayBuffer == 0)
			{
				// @@ TODO: error message
				b = false;
			}

			if (doConvertPlayBuffer)
			{
				nb = bufferSize;
				nb *= nDevPlayChannels;
				nb *= nBytes(devPlayFormat);
				devPlayBuffer = new char[nb];

				if (devPlayBuffer == 0)
				{
					// @@ TODO: error message
					b = false;
				}
			}
		}

		// Setup the buffer conversion information structure.  We don't use buffers to do channel offsets,
		// so we override that parameter here.

		static constexpr int firstChannel = 0; // @@ TODO: make parm?

		if (doConvertRecBuffer)
		{
			cfgRecConvertInfo(firstChannel);
		}

		if (doConvertPlayBuffer)
		{
			cfgPlayConvertInfo(firstChannel);
		}

		return b;
	}

	void DfxStream::cfgPlayConvertInfo(unsigned int firstChannel)
	{
		// convert user to device buffer
		convertPlayInfo.inJump = nUserPlayChannels;
		convertPlayInfo.outJump = nDevPlayChannels;
		convertPlayInfo.inFormat = userFormat;
		convertPlayInfo.outFormat = devPlayFormat;

		if (convertPlayInfo.inJump < convertPlayInfo.outJump)
		{
			convertPlayInfo.nChannels = convertPlayInfo.inJump;
		}
		else
		{
			convertPlayInfo.nChannels = convertPlayInfo.outJump;
		}

		// Set up the interleave/deinterleave offsets.

		if (devPlayInterleaved != userInterleaved)
		{
			if (devPlayInterleaved)
			{
				for (int k = 0; k < convertPlayInfo.nChannels; k++)
				{
					convertPlayInfo.inOffset.push_back(k * bufferSize);
					convertPlayInfo.outOffset.push_back(k);
					convertPlayInfo.inJump = 1;
				}
			}
			else 
            {
				for (int k = 0; k < convertPlayInfo.nChannels; k++)
				{
					convertPlayInfo.inOffset.push_back(k);
					convertPlayInfo.outOffset.push_back(k * bufferSize);
					convertPlayInfo.outJump = 1;
				}
			}
		}
		else
		{
			// no (de)interleaving

			if (userInterleaved)
			{
				for (int k = 0; k < convertPlayInfo.nChannels; k++)
				{
					convertPlayInfo.inOffset.push_back(k);
					convertPlayInfo.outOffset.push_back(k);
				}
			}
			else
			{
				for (int k = 0; k < convertPlayInfo.nChannels; k++)
				{
					convertPlayInfo.inOffset.push_back(k * bufferSize);
					convertPlayInfo.outOffset.push_back(k * bufferSize);
					convertPlayInfo.inJump = 1;
					convertPlayInfo.outJump = 1;
				}
			}
		}

		// Add channel offset.

		if (firstChannel > 0) {

			if (devPlayInterleaved)
			{
				for (int k = 0; k < convertPlayInfo.nChannels; k++)
				{
					convertPlayInfo.outOffset[k] += firstChannel;
				}
			}
			else
			{
				for (int k = 0; k < convertPlayInfo.nChannels; k++)
				{
					convertPlayInfo.outOffset[k] += (firstChannel * bufferSize);
				}
			}
		}
	}

	void DfxStream::cfgRecConvertInfo(unsigned int firstChannel)
	{
		// convert device to user buffer

		convertRecInfo.inJump = nDevRecChannels;
		convertRecInfo.outJump = nUserRecChannels;
		convertRecInfo.inFormat = devRecFormat;
		convertRecInfo.outFormat = userFormat;

		if (convertRecInfo.inJump < convertRecInfo.outJump)
		{
			convertRecInfo.nChannels = convertRecInfo.inJump;
		}
		else
		{
			convertRecInfo.nChannels = convertRecInfo.outJump;
		}

		// Set up the interleave/deinterleave offsets.
		if (devRecInterleaved != userInterleaved)
		{
			if (userInterleaved)
			{
				for (int k = 0; k < convertRecInfo.nChannels; k++)
				{
					convertRecInfo.inOffset.push_back(k * bufferSize);
					convertRecInfo.outOffset.push_back(k);
					convertRecInfo.inJump = 1;
				}
			}
			else
			{
				for (int k = 0; k < convertRecInfo.nChannels; k++)
				{
					convertRecInfo.inOffset.push_back(k);
					convertRecInfo.outOffset.push_back(k * bufferSize);
					convertRecInfo.outJump = 1;
				}
			}
		}
		else 
		{ 
			// no (de)interleaving

			if (userInterleaved)
			{
				for (int k = 0; k < convertRecInfo.nChannels; k++)
				{
					convertRecInfo.inOffset.push_back(k);
					convertRecInfo.outOffset.push_back(k);
				}
			}
			else
			{
				for (int k = 0; k < convertRecInfo.nChannels; k++)
				{
					convertRecInfo.inOffset.push_back(k * bufferSize);
					convertRecInfo.outOffset.push_back(k * bufferSize);
					convertRecInfo.inJump = 1;
					convertRecInfo.outJump = 1;
				}
			}
		}

		// Add channel offset.
		if (firstChannel > 0)
		{
			if (devRecInterleaved)
			{
				for (int k = 0; k < convertRecInfo.nChannels; k++)
				{
					convertRecInfo.inOffset[k] += firstChannel;
				}
			}
			else 
			{
				for (int k = 0; k < convertRecInfo.nChannels; k++)
				{
					convertRecInfo.inOffset[k] += (firstChannel * bufferSize);
				}
			}
		}
	}

    void convertBuffer(DfxStream &stream, char* outBuffer, char* inBuffer, ConvertInfo& info)
    {
        // This function does format conversion, input/output channel compensation.

        const unsigned nSamples = stream.bufferSize;
        const int nChannels = info.nChannels;
        const int inStride = info.nChannels;
        const int outStride = info.nChannels;

        convertBuffer(info.outFormat, outBuffer, outStride, info.inFormat, inBuffer, inStride, nSamples, nChannels);
    }

#if 0
    void convertBufferX(DfxStream& stream, char* outBuffer, char* inBuffer, ConvertInfo& info)
    {
        // This function does format conversion, input/output channel compensation, and
        // data interleaving/deinterleaving.

        const unsigned nSamples = stream.bufferSize;
        const int nChannels = info.nChannels;
        const int inStride = info.inJump;
        const int outStride = info.outJump;

        // The code below is very similar to that in the SampleUtil / convertBuffer() routine.
        // All that's different is that you can start from some channel other than the first.
        // That's what the offset stuff is all about.

        switch (info.outFormat)
        {
            case SampleFormat::SINT16:
            {
                auto* out = (int16_t*)outBuffer;

                switch (info.inFormat)
                {
                    case SampleFormat::SINT16:
                    {
                        // Channel compensation and/or (de)interleaving only.
                        auto in = (int16_t*)inBuffer;
                        for (unsigned i = 0; i < nSamples; i++)
                        {
                            for (int j = 0; j < nChannels; j++)
                            {
                                out[info.outOffset[j]] = in[info.inOffset[j]];
                            }
                            in += inStride;
                            out += outStride;
                        }
                    }
                    break;

                    case SampleFormat::SINT24:
                    {
                        auto in = (int24_t*)inBuffer;
                        for (unsigned i = 0; i < nSamples; i++)
                        {
                            for (int j = 0; j < nChannels; j++)
                            {
                                //out[info.outOffset[j]] = (int16_t)(in[info.inOffset[j]].asInt() >> 8); // If 24-bit data is in lower three bytes of int32_t
                                out[info.outOffset[j]] = (int16_t)(in[info.inOffset[j]].asInt() >> 16); // If 24-bit data is in upper three bytes of int32_t
                            }
                            in += inStride;
                            out += outStride;
                        }
                    }
                    break;

                    case SampleFormat::SINT32:
                    {
                        auto in = (int32_t*)inBuffer;
                        for (unsigned i = 0; i < nSamples; i++)
                        {
                            for (int j = 0; j < nChannels; j++)
                            {
                                out[info.outOffset[j]] = (int16_t)((in[info.inOffset[j]] >> 16) & 0x0000ffff);
                            }
                            in += inStride;
                            out += outStride;
                        }
                    }
                    break;

                    case SampleFormat::FLOAT32:
                    {
                        auto in = (float*)inBuffer;
                        for (unsigned i = 0; i < nSamples; i++)
                        {
                            for (int j = 0; j < nChannels; j++)
                            {
                                out[info.outOffset[j]] = (int16_t)(in[info.inOffset[j]] * 32767.5 - 0.5);
                            }
                            in += inStride;
                            out += outStride;
                        }
                    }
                    break;

                    case SampleFormat::FLOAT64:
                    {
                        auto in = (double*)inBuffer;
                        for (unsigned i = 0; i < nSamples; i++)
                        {
                            for (int j = 0; j < nChannels; j++) {
                                out[info.outOffset[j]] = (int16_t)(in[info.inOffset[j]] * 32767.5 - 0.5);
                            }
                            in += inStride;
                            out += outStride;
                        }
                    }
                }
            }
            break;

            case SampleFormat::SINT24:
            {
                auto out = (int24_t*)outBuffer;

                switch (info.inFormat)
                {
                    case SampleFormat::SINT16:
                    {
                        auto in = (int16_t*)inBuffer;
                        for (unsigned i = 0; i < nSamples; i++)
                        {
                            for (int j = 0; j < nChannels; j++)
                            {
                                out[info.outOffset[j]] = (int32_t)(in[info.inOffset[j]] << 8);
                                //out[info.outOffset[j]] <<= 8;
                            }
                            in += inStride;
                            out += outStride;
                        }
                    }
                    break;

                    case SampleFormat::SINT24:
                    {
                        // Channel compensation and/or (de)interleaving only.
                        auto in = (int24_t*)inBuffer;
                        for (unsigned i = 0; i < nSamples; i++)
                        {
                            for (int j = 0; j < nChannels; j++) {
                                out[info.outOffset[j]] = in[info.inOffset[j]];
                            }
                            in += inStride;
                            out += outStride;
                        }
                    }
                    break;

                    case SampleFormat::SINT32:
                    {
                        auto in = (int32_t*)inBuffer;
                        for (unsigned i = 0; i < nSamples; i++)
                        {
                            for (int j = 0; j < nChannels; j++)
                            {
                                out[info.outOffset[j]] = (int32_t)(in[info.inOffset[j]] >> 8);
                                //out[info.outOffset[j]] >>= 8;
                            }
                            in += inStride;
                            out += outStride;
                        }
                    }
                    break;

                    case SampleFormat::FLOAT32:
                    {
                        auto in = (float*)inBuffer;
                        for (unsigned i = 0; i < nSamples; i++) {
                            for (int j = 0; j < nChannels; j++) {
                                out[info.outOffset[j]] = (int32_t)(in[info.inOffset[j]] * 8388607.5 - 0.5);
                            }
                            in += inStride;
                            out += outStride;
                        }
                    }
                    break;

                    case SampleFormat::FLOAT64:
                    {
                        auto in = (double*)inBuffer;
                        for (unsigned i = 0; i < nSamples; i++) {
                            for (int j = 0; j < nChannels; j++) {
                                out[info.outOffset[j]] = (int32_t)(in[info.inOffset[j]] * 8388607.5 - 0.5);
                            }
                            in += inStride;
                            out += outStride;
                        }
                    }
                }
            }
            break;

            case SampleFormat::SINT32:
            {
                auto out = (int32_t*)outBuffer;

                switch (info.inFormat)
                {

                    case SampleFormat::SINT16:
                    {
                        auto in = (int16_t*)inBuffer;
                        for (unsigned i = 0; i < nSamples; i++)
                        {
                            for (int j = 0; j < nChannels; j++)
                            {
                                auto& d = out[info.outOffset[j]];
                                d = (int32_t)in[info.inOffset[j]];
                                d <<= 16;
                            }
                            in += inStride;
                            out += outStride;
                        }
                    }
                    break;

                    case SampleFormat::SINT24:
                    {
                        auto in = (int24_t*)inBuffer;
                        for (unsigned i = 0; i < nSamples; i++)
                        {
                            for (int j = 0; j < nChannels; j++)
                            {
                                auto& d = out[info.outOffset[j]];
                                d = (int32_t)in[info.inOffset[j]].asInt();
                                //d <<= 8; If 24-bit data is in lower three bytes of int32_t.
                            }
                            in += inStride;
                            out += outStride;
                        }
                    }
                    break;

                    case SampleFormat::SINT32:
                    {
                        // Channel compensation and/or (de)interleaving only.
                        auto in = (int32_t*)inBuffer;
                        for (unsigned i = 0; i < nSamples; i++)
                        {
                            for (int j = 0; j < nChannels; j++)
                            {
                                out[info.outOffset[j]] = in[info.inOffset[j]];
                            }
                            in += inStride;
                            out += outStride;
                        }
                    }
                    break;

                    case SampleFormat::FLOAT32:
                    {
                        auto in = (float*)inBuffer;
                        for (unsigned i = 0; i < nSamples; i++)
                        {
                            for (int j = 0; j < nChannels; j++)
                            {
                                out[info.outOffset[j]] = (int32_t)(in[info.inOffset[j]] * 2147483647.5 - 0.5);
                            }
                            in += inStride;
                            out += outStride;
                        }
                    }
                    break;

                    case SampleFormat::FLOAT64:
                    {
                        auto in = (double*)inBuffer;
                        for (unsigned i = 0; i < nSamples; i++)
                        {
                            for (int j = 0; j < nChannels; j++)
                            {
                                out[info.outOffset[j]] = (int32_t)(in[info.inOffset[j]] * 2147483647.5 - 0.5);
                            }
                            in += inStride;
                            out += outStride;
                        }
                    }
                }
            }
            break;

            case SampleFormat::FLOAT32:
            {
                float scale;
                auto out = (float*)outBuffer;

                switch (info.inFormat)
                {

                    case SampleFormat::SINT16:
                    {
                        auto in = (int16_t*)inBuffer;
                        scale = (float)(1.0 / 32767.5);
                        for (unsigned i = 0; i < nSamples; i++)
                        {
                            for (int j = 0; j < nChannels; j++)
                            {
                                auto& d = out[info.outOffset[j]];
                                d = (float)in[info.inOffset[j]];
                                d += 0.5f; // @@ BRY
                                d *= scale;
                            }
                            in += inStride;
                            out += outStride;
                        }
                    }
                    break;

                    case SampleFormat::SINT24:
                    {
                        auto in = (int24_t*)inBuffer;
                        //scale = (float)(1.0 / 8388607.5);  // If 24-bit data is in lower three bytes of int32_t.
                        scale = (float)(1.0 / 2147483647.5); // If 24-bit data is in upper three bytes of int32_t.
                        for (unsigned i = 0; i < nSamples; i++)
                        {
                            for (int j = 0; j < nChannels; j++)
                            {
                                auto& d = out[info.outOffset[j]];
                                d = in[info.inOffset[j]].asFloat(); // @@ asFloat has subtle casting
                                d += 0.5f; // @@ BRY
                                d *= scale;
                            }
                            in += inStride;
                            out += outStride;
                        }
                    }
                    break;

                    case SampleFormat::SINT32:
                    {
                        auto in = (int32_t*)inBuffer;
                        scale = (float)(1.0 / 2147483647.5);
                        for (unsigned i = 0; i < nSamples; i++)
                        {
                            for (int j = 0; j < nChannels; j++)
                            {
                                auto& d = out[info.outOffset[j]];
                                d = (float)in[info.inOffset[j]];
                                d += 0.5f; // @@ BRY
                                d *= scale;
                            }
                            in += inStride;
                            out += outStride;
                        }
                    }
                    break;

                    case SampleFormat::FLOAT32:
                    {
                        // Channel compensation and/or (de)interleaving only.
                        auto in = (float*)inBuffer;
                        for (unsigned i = 0; i < nSamples; i++)
                        {
                            for (int j = 0; j < nChannels; j++)
                            {
                                out[info.outOffset[j]] = in[info.inOffset[j]];
                            }
                            in += inStride;
                            out += outStride;
                        }
                    }
                    break;

                    case SampleFormat::FLOAT64:
                    {
                        auto in = (double*)inBuffer;
                        for (unsigned i = 0; i < nSamples; i++)
                        {
                            for (int j = 0; j < nChannels; j++)
                            {
                                out[info.outOffset[j]] = (float)in[info.inOffset[j]];
                            }
                            in += inStride;
                            out += outStride;
                        }
                    }
                }
            }
            break;

            case SampleFormat::FLOAT64:
            {
                double scale;
                auto* out = (double*)outBuffer;

                switch (info.inFormat)
                {
                    case SampleFormat::SINT16:
                    {
                        auto in = (int16_t*)inBuffer;
                        scale = 1.0 / 32767.5;

                        for (unsigned i = 0; i < nSamples; i++)
                        {
                            for (int j = 0; j < nChannels; j++)
                            {
                                auto& d = out[info.outOffset[j]];
                                d = (double)in[info.inOffset[j]];
                                d += 0.5;
                                d *= scale;
                            }
                            in += inStride;
                            out += outStride;
                        }
                    }
                    break;

                    case SampleFormat::SINT24:
                    {
                        auto in = (int24_t*)inBuffer;
                        //scale = 1.0 / 8388607.5;  // If 24-bit data is in lower three bytes of int32_t
                        scale = 1.0 / 2147483647.5; // If 24-bit data is in upper three bytes of int32_t
                        for (unsigned i = 0; i < nSamples; i++)
                        {
                            for (int j = 0; j < nChannels; j++)
                            {
                                auto& d = out[info.outOffset[j]];
                                d = double(in[info.inOffset[j]].asInt());
                                d += 0.5;
                                d *= scale;
                            }
                            in += inStride;
                            out += outStride;
                        }
                    }
                    break;

                    case SampleFormat::SINT32:
                    {
                        auto in = (int32_t*)inBuffer;
                        scale = 1.0 / 2147483647.5;
                        for (unsigned i = 0; i < nSamples; i++)
                        {
                            for (int j = 0; j < nChannels; j++)
                            {
                                auto& d = out[info.outOffset[j]];
                                d = (double)in[info.inOffset[j]];
                                d += 0.5;
                                d *= scale;
                            }
                            in += inStride;
                            out += outStride;
                        }
                    }
                    break;

                    case SampleFormat::FLOAT32:
                    {
                        auto in = (float*)inBuffer;
                        for (unsigned i = 0; i < nSamples; i++)
                        {
                            for (int j = 0; j < nChannels; j++)
                            {
                                out[info.outOffset[j]] = (double)in[info.inOffset[j]];
                            }
                            in += inStride;
                            out += outStride;
                        }
                    }
                    break;

                    case SampleFormat::FLOAT64:
                    {
                        // Channel compensation and/or (de)interleaving only.
                        auto in = (double*)inBuffer;
                        for (unsigned i = 0; i < nSamples; i++)
                        {
                            for (int j = 0; j < nChannels; j++)
                            {
                                out[info.outOffset[j]] = in[info.inOffset[j]];
                            }
                            in += inStride;
                            out += outStride;
                        }
                    }
                    break;
                }
            }
            break;

        }
    }

#endif

} // end of namaepsace
