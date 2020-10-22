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

#include "AsioMgr.h"
#include "InternalAsioMgr.h"
#include <iostream>

namespace dfx
{
	// ////////////////////////////////////////////////////////////////////////
	// 
	// 
	// ////////////////////////////////////////////////////////////////////////


	AsioMgr::AsioMgr()
	: DfxAudio()
	{
		// Link to the static internal asio manager
		iam = static_iam;
	}

	AsioMgr::~AsioMgr()
	{
		closeStream();
		iam = nullptr;
	}

	int AsioMgr::LastError()
	{
		// TBD
		return 0;
	}

	bool AsioMgr::LoadDriver(const std::string& name)
	{
		return iam->LoadDriver(name);
	}

	bool AsioMgr::InitDriver(bool verbose)
	{
		bool b = iam->InitDriver(verbose);

		if (b)
		{
			b = QueryDriverInfo(verbose);
		}

		if (b)
		{
			b = QueryDeviceInfo(verbose);
		}

		return b;
	}

	void AsioMgr::UnloadDriver()
	{
		return iam->UnloadDriver();
	}

	long AsioMgr::NumDevices()
	{
		return iam->NumDevices();
	}

	std::vector<std::string> AsioMgr::DeviceNames()
	{
		return iam->DeviceNames();
	}

	std::string AsioMgr::DeviceName(long devId)
	{
		return iam->DeviceName(devId);
	}

	long AsioMgr::QueryDeviceID()
	{
		return iam->QueryDeviceID();
	}


	bool AsioMgr::Start()
	{
		bool b = iam->Start();
		stream.state = StreamState::RUNNING;
		return b;
	}

	bool AsioMgr::Stop()
	{
		bool b = iam->Stop();
		stream.state = StreamState::STOPPED;
		return b;
	}

	bool AsioMgr::Stopped()
	{
		// @@ Hmmm....return iam->Stopped();
		return stream.state == StreamState::STOPPED;
	}

	bool AsioMgr::Exit()
	{
		return iam->Exit();
	}

	bool AsioMgr::QueryDriverInfo(bool verbose)
	{
		return iam->QueryDriverInfo(verbose);
	}

	bool AsioMgr::QueryDeviceInfo(bool verbose)
	{
		return iam->QueryDeviceInfo(devInfo, verbose);
	}

	void AsioMgr::closeStream()
	{
		if (stream.state == StreamState::CLOSED) 
		{
			return;
		}

		if (stream.state == StreamState::RUNNING) 
		{
			stream.state = StreamState::STOPPED;
			ASIOStop();
		}

		DisposeBuffers();

		UnloadDriver();

		AsioHandle* handle = (AsioHandle*)stream.apiHandle;

		if (handle) 
		{
			CloseHandle(handle->condition);

			// @@ NO! handle->bufferInfos has already been
			// deallocated. Do NOT do that here!

			//if (handle->bufferInfos)
			//{
				//delete handle->bufferInfos;
			//}

			delete handle;
			stream.apiHandle = 0;
		}

		DfxAudio::closeStream(); // Gets rid of stream buffers
	}

	bool stopThreadCalled = false;

	void AsioMgr::startStream()
	{
		verifyStream();

		if (stream.state == StreamState::RUNNING) 
		{
			//errorText_ = "startStream(): the stream is already running!";
			//error(RtAudioError::WARNING);
			return;
		}

#if defined( HAVE_GETTIMEOFDAY )
		gettimeofday(&stream_.lastTickTimestamp, NULL);
#endif

		AsioHandle* handle = (AsioHandle*)stream.apiHandle;
		ASIOError result = ASIOStart();

		if (result != ASE_OK) 
		{
			//errorStream_ << "startStream: error (" << getAsioErrorString(result) << ") starting device.";
			//errorText_ = errorStream_.str();
			goto unlock;
		}

		handle->drainCounter = 0;
		handle->internalDrain = false;
		ResetEvent(handle->condition);

		stream.state = StreamState::RUNNING;
		InternalAsioMgr::asioXRun = false;

	unlock:
		stopThreadCalled = false;

		//if (result == ASE_OK) return;
		//error(RtAudioError::SYSTEM_ERROR);

		return;
	}


	void AsioMgr::stopStream()
	{
		verifyStream();

		if (stream.state == StreamState::STOPPED) 
		{
			return;
		}

		AsioHandle* handle = (AsioHandle*)stream.apiHandle;

		if (stream.nDevPlayChannels > 0)
		{
			if (handle->drainCounter == 0) 
			{
				handle->drainCounter = 2;
				// @@ TODO: Why are we doing this? 
				WaitForSingleObject(handle->condition, INFINITE);  // block until signaled
			}
		}

		stream.state = StreamState::STOPPED;

		bool result = iam->Stop();

		//if (result != ASE_OK) 
		if (!result)
		{
			//errorStream_ << "stopStream: error (" << getAsioErrorString(result) << ") stopping device.";
			//errorText_ = errorStream_.str();
		}

		//if (result == ASE_OK) return;
		//error(RtAudioError::SYSTEM_ERROR);

		return;
	}

	void AsioMgr::abortStream()
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

	// This function will be called by a spawned thread when the user
	// callback function signals that the stream should be stopped or
	// aborted.  It is necessary to handle it this way because the
	// callbackEvent() function must return before the ASIOStop()
	// function will return.
	static unsigned __stdcall asioStopStream(void* ptr)
	{
		CallbackInfo* info = (CallbackInfo*)ptr;
		auto object = (AsioMgr*)info->object;

		object->stopStream();
		_endthreadex(0);
		return 0;
	}


	//
	///////////////////////////////////////////////////////////

	static ASIOCallbacks asioCallbacks; // This has to hang around while stream is running

	static CallbackInfo *asioCallbackInfo;

#if 0
	static ASIOTime* bufferSwitchTimeInfo(ASIOTime* timeInfo, long index, ASIOBool processNow)
	{
		static long processedSamples = 0;

		// store the timeInfo for later use
		driverData.tInfo = *timeInfo;

		// Get the time stamp of the buffer. This is not necessary if no
		// synchronization to other media is required

		if (timeInfo->timeInfo.flags & kSystemTimeValid)
		{
			driverData.nanoSeconds = ASIO64toDouble(timeInfo->timeInfo.systemTime);
		}
		else
		{
			driverData.nanoSeconds = 0;
		}

		if (timeInfo->timeInfo.flags & kSamplePositionValid)
		{
			driverData.samplePos = ASIO64toDouble(timeInfo->timeInfo.samplePosition);
		}
		else
		{
			driverData.samplePos = 0;
		}

		if (timeInfo->timeCode.flags & kTcValid)
		{
			driverData.tcSamples = ASIO64toDouble(timeInfo->timeCode.timeCodeSamples);
		}
		else
		{
			driverData.tcSamples = 0;
		}

		// get the system reference time
		driverData.sysRefTime = get_sys_reference_time();

#if WINDOWS && _DEBUG
		// a few debug messages for the Windows device driver developer
		// tells you the time when driver got its interrupt and the delay until the app receives
		// the event notification.
		static double last_samples = 0;

		std::stringstream str;

		str << " / " << driverData.sysRefTime;
		str << " ms / " << (long)(driverData.nanoSeconds / 1000000.0);
		str << " ms / " << (long)(driverData.samplePos - last_samples);
		str << " samples                 ";

		std::string fred = str.str();
		//OutputDebugString(fred);
		//std::cout << str.str();
		last_samples = driverData.samplePos;
#endif

		bufferSwitch(index, processNow);

		processedSamples += buffSize;
	}

#endif

	void AsioMgr::bufferSwitch(long index, long processNow)
	{
		// Static member function
		// Note: long processNow is really ASIOBool processNow

		// asioCallbackInfo must be static
		auto object = reinterpret_cast<AsioMgr*>(asioCallbackInfo->object);

		// @@ HACK
		// get the system reference time
		object->iam->driverData.sysRefTime = get_sys_reference_time();

		object->callbackEvent(index);

		// @@ HACK
		// From tickStreamTime(): stream.streamTime += (stream.bufferSize * 1.0 / stream.sampleRate);

		//object->iam->driverData.samplePos = object->getStreamTime() * object->;
	}

	void AsioMgr::sampleRateChanged(double sRate)
	{
		// Static member function
		// Note: double sRate is really ASIOSampleRate srate

		// Callback

		// Do whatever you need to do if the sample rate changed
		// usually this only happens during external sync.
		// Audio processing is not stopped by the driver, actual sample rate
		// might not have even changed, maybe only the sample rate status of an
		// AES/EBU or S/PDIF digital input at the audio device.
		// You might have to update time/sample related conversion routines, etc.
	}

	long AsioMgr::asioMessage(long selector, long value, void* message, double* opt)
	{
		// Static member function

		// Callback

		// Currently the parameters "message" and "opt" are not used.

		long ret = 0;
		switch (selector)
		{
			case kAsioSelectorSupported:
			{
				if (value == kAsioResetRequest
					|| value == kAsioEngineVersion
					|| value == kAsioResyncRequest
					|| value == kAsioLatenciesChanged
					// The following three were added for ASIO 2.0. You don't necessarily have to support them
					|| value == kAsioSupportsTimeInfo
					|| value == kAsioSupportsTimeCode
					|| value == kAsioSupportsInputMonitor)
				{
					ret = 1L;
				}
				break;
			}

			case kAsioResetRequest:
			{
				// Defer the task and perform the reset of the driver during the next "safe" situation
				// You cannot reset the driver right now, as this code is called from the driver.
				// Reset the driver is done by completely destruct is. I.e. ASIOStop(), ASIODisposeBuffers(), Destruction
				// Afterwards you initialize the driver again.

				InternalAsioMgr::driverData.stopped;  // In this sample the processing will just stop
				ret = 1L;
				break;
			}

			case kAsioResyncRequest:
			{
				// This informs the application, that the driver encountered some non fatal data loss.
				// It is used for synchronization purposes of different media.
				// Added mainly to work around the Win16Mutex problems in Windows 95/98 with the
				// Windows Multimedia system, which could loose data because the Mutex was hold too
				// long by another thread.
				// However a driver can issue it in other situations, too.

				InternalAsioMgr::asioXRun = true;
				ret = 1L;
				break;
			}

			case kAsioLatenciesChanged:
			{
				// This will inform the host application that the drivers were latencies changed.
				// Beware, it this does not mean that the buffer sizes have changed!
				// You might need to update internal delay data.

				ret = 1L;
				break;
			}

			case kAsioEngineVersion:
			{
				// Return the supported ASIO version of the host application.
				// If a host applications does not implement this selector, 
				// ASIO 1.0 is assumed by the driver.

				ret = 2L;
				break;
			}

			case kAsioSupportsTimeInfo:
			{
				// Informs the driver whether the asioCallbacks.bufferSwitchTimeInfo() callback
				// is supported.

				// For compatibility with ASIO 1.0 drivers the host application should always support
				// the "old" bufferSwitch method, too.

				//ret = 1L;
				ret = 0L; // @@ TODO: FOR NOW
				break;
			}

			case kAsioSupportsTimeCode:
			{
				// Informs the driver whether application is interested in time code info.
				// If an application does not need to know about time code, the driver has less work
				// to do.
				ret = 0L;
				break;
			}

			case kAsioOverload:
				InternalAsioMgr::asioXRun = true;
				return 0L; // Return value actually ignored.
			break;

			default:
			{
				throw std::exception("asioMessages(): Invalid switch statement selector");
			}
		}

		return ret;
	}


	//
	///////////////////////////////////////////////////////////

	bool AsioMgr::Open(long nInputChannels_, long nOutputChannels_, long bufferSize_, unsigned sampleRate_, void *userData_, bool verbose)
	{
		// If number of channels is 0 in one direction, it means, obviously, no communication that way

		//stream.mode = mode;

		stream.state = StreamState::STOPPED;

		//stream.sampleRate = static_cast<int>(iam->driverData.sampleRate); // @@ TODO: Can set? Or, devInfo.preferredSampleRate;

		if (sampleRate_ == 0)
		{
			sampleRate_ = devInfo.preferredSampleRate;
		}
		else
		{
			bool b = devInfo.IsCompatibleSampleRate(sampleRate_);

			if (!b)
			{
				sampleRate_ = devInfo.preferredSampleRate;
			}
		}

		ASIOSetSampleRate(sampleRate_); // @@ TODO: check success

		iam->driverData.sampleRate = sampleRate_; // @@ is this what we want to do?

		stream.sampleRate = sampleRate_;

		stream.bufferSize = bufferSize_; // @@ TODO: Check compatibility

		stream.callbackInfo.object = this;
		asioCallbackInfo = &stream.callbackInfo; // Set static pointer
		stream.callbackInfo.userData = userData_;

		//

		asioCallbacks.asioMessage = iam->asioMessage;
		asioCallbacks.sampleRateDidChange = sampleRateChanged;
		asioCallbacks.bufferSwitchTimeInfo = nullptr; // bufferSwitchTimeInfo; //  nullptr;
		asioCallbacks.bufferSwitch = bufferSwitch; //  iam->bufferSwitch; //  bufferSwitch; // Local

		// @@ TODO: Move much of this functionality as possible out of InternalAsioMgr. Only do those things
		// there that need the internal stuff to function

		bool b = iam->CreateAsioBuffers(nInputChannels_, nOutputChannels_, stream.bufferSize, &asioCallbacks, verbose);

		if (!b)
		{
			return false;
		}

		// Only do these after opening, so we have right number of channels

		stream.nUserPlayChannels = iam->driverData.nOutputChannels;
		stream.nUserRecChannels = iam->driverData.nInputChannels;

		stream.nDevPlayChannels = iam->driverData.nOutputChannels;
		stream.nDevRecChannels = iam->driverData.nInputChannels;

		// Only valid after creating the asio buffers

		stream.playLatency = iam->driverData.outputLatency;
		stream.recLatency = iam->driverData.inputLatency;

		// ////////////////////////////

		stream.devPlayID = iam->QueryDeviceID();
		stream.devRecID = iam->QueryDeviceID();

		if (verbose)
		{
			std::cout << std::endl;
			std::cout << "Opening device " << DeviceName(stream.devPlayID) << "( DevID" << stream.devPlayID << ")" << std::endl;
			std::cout << "Sample rate = " << stream.sampleRate << " Hz" << std::endl;

			unsigned tot_latency = 0;

			if (stream.nUserPlayChannels > 0)
			{
				std::cout << "Using playback: ";
				std::cout << "Num channels = " << stream.nUserPlayChannels << ", latency = " << stream.playLatency << " samples" << std::endl;
				tot_latency += stream.playLatency;
			}

			if (stream.nUserRecChannels > 0)
			{
				std::cout << "Using recording: ";
				std::cout << "Num channels = " << stream.nUserRecChannels << ", recording latency = " << stream.recLatency << " samples " << std::endl;
				tot_latency += stream.recLatency;
			}

			std::cout << "Total device latency = " << tot_latency << " samples ";
			
			double tlat = (tot_latency * 1000.0) / stream.sampleRate;

			std::cout << "(" << tlat << " msec)" << std::endl;
		}

		stream.userInterleaved = true; // @@TODO: someday flexible?

		stream.devPlayInterleaved = false; // always false for ASIO
		stream.devRecInterleaved = false;  // always false for ASIO

#ifdef __LITTLE_ENDIAN__
		if (devInfo.little_endian)
		{
			stream.swapPlayBytes = false;
			stream.swapRecBytes = false;
		}
		else
		{
			stream.swapPlayBytes = true;
			stream.swapRecBytes = true;
		}
#else
		if (devInfo.little_endian)
		{
			stream.swapPlayBytes = true;
			stream.swapRecBytes = true;
		}
		else
		{
			stream.swapPlayBytes = false;
			stream.swapRecBytes = false;
		}
#endif

		stream.userFormat = system_fmt;  // @@ TODO Someday flexible?

		stream.devPlayFormat = devInfo.format;
		stream.devRecFormat = devInfo.format;

		// Finish all platform independent stream stuff

		b = stream.FinishBufferConfig();

		if (b)
		{
			auto handle = reinterpret_cast<AsioHandle*>(stream.apiHandle);

			delete handle;
			handle = new AsioHandle;

			if (handle)
			{
				handle->bufferInfos = iam->driverData.bufferInfos;

				handle->condition = CreateEvent(
					nullptr, // no security
					TRUE,    // manual reset
					FALSE,   // non-signaled initially
					nullptr  // unnamed
				);

				stream.apiHandle = handle;
			}
			else b = false;
		}

		return b;
	}


	bool AsioMgr::callbackEvent(long bufferIndex)
	{
		if (stream.state == StreamState::STOPPED || stream.state == StreamState::STOPPING) return true;

		if (stream.state == StreamState::CLOSED) 
		{
			// @@ TODO:
			//errorText_ = "RtApiAsio::callbackEvent(): the stream is closed ... this shouldn't happen!";
			//error(RtAudioError::WARNING);
			return false;
		}

		CallbackInfo* info = (CallbackInfo*)&stream.callbackInfo;
		AsioHandle* handle = (AsioHandle*)stream.apiHandle;

		// Check if we were draining the stream and signal if finished.
		if (handle->drainCounter > 3) 
		{
			stream.state = StreamState::STOPPING;

			if (handle->internalDrain == false)
			{
				SetEvent(handle->condition);
			}
			else 
			{ 
				// This function will be called by a spawned thread when the user
				// callback function signals that the stream should be stopped or
				// aborted.  It is necessary to handle it this way because the
				// callbackEvent() function must return before the ASIOStop()
				// function will return.

				// spawn a thread to stop the stream

				unsigned threadId;
				//stream.callbackInfo.thread = _beginthreadex(NULL, 0, &asioStopStream, &stream.callbackInfo, 0, &threadId);

				_beginthreadex(NULL, 0, &asioStopStream, &stream.callbackInfo, 0, &threadId);

				// Much simpler now. @@ Except causes an exception!
				//std::thread bare([&] { this->stopStream(); });
				//std::thread bare([&] { asioStopStream(&stream.callbackInfo); });
			}
			return true;
		}

		// /////////////////////////////////////////////////////////////////////////////
		// Let's get input data from device and convert into internal form
		// /////////////////////////////////////////////////////////////////////////////

		//if (stream.mode == INPUT || stream.mode == DUPLEX) 
		if (stream.nDevRecChannels > 0)
		{
			unsigned nBytesPerSample = nBytes(stream.devRecFormat);
			unsigned nBytesPerChannel = stream.bufferSize * nBytesPerSample;

			unsigned nChannels = stream.nDevPlayChannels + stream.nDevRecChannels;

			if (stream.doConvertRecBuffer)
			{
				// ASIO data always comes in non-interleaved.
				// We'd like it to be converted to interleaved, because reasons.
				// Can we do it?

				for (unsigned c = 0, rel_c = 0; c < nChannels; c++)
				{
					auto &cbi = handle->bufferInfos[c];
			
					if (cbi.isInput == ASIOTrue)
					{
						auto pChannelBytes = cbi.buffers[bufferIndex];
#if 0
						// This leaves it non-interleaved
						memcpy(&stream.devRecBuffer[nBytesPerChannel * rel_c++], pChannelBytes, nBytesPerChannel);
#else
						// Interleaves and swaps bytes if necessary
						InterleaveChannel(stream.devRecFormat, stream.devRecBuffer, pChannelBytes, rel_c++, stream.nDevRecChannels, stream.bufferSize, stream.swapRecBytes);
#endif
					}
				}
				
#if 0
				if (stream.swapRecBytes) 
				{
					byteSwapBuffer(stream.devRecFormat, stream.devRecBuffer, stream.bufferSize * stream.nDevRecChannels);
				}
#endif

#if 0
				// Not only converts, but also interleaves the user channels
				convertBufferX(stream, stream.userRecBuffer, stream.devRecBuffer, stream.convertRecInfo);
#else
				// Only converts
				convertBuffer(stream, stream.userRecBuffer, stream.devRecBuffer, stream.convertRecInfo);
#endif
			}
			else
			{
				// Don't have to worry about interleaving or type conversion.
				// May have to swap bytes though.

				for (unsigned c = 0, rel_c = 0; c < nChannels; c++)
				{
					auto& cbi = handle->bufferInfos[c];

					if (cbi.isInput == ASIOTrue)
					{
						auto dest = &stream.userRecBuffer[nBytesPerChannel * rel_c++];
						auto pChannelBytes = cbi.buffers[bufferIndex];
						if (stream.swapRecBytes)
						{
							CopyByteSwap(stream.userFormat, dest, pChannelBytes, stream.bufferSize, 1, 1);
						}
						else
						{
							memcpy(dest, pChannelBytes, nBytesPerChannel);
						}
					}
				}

				//if (stream.swapRecBytes)
				//{
				//	byteSwapBuffer(stream.userFormat, stream.userRecBuffer, stream.bufferSize * stream.nUserRecChannels);
				//}
			}
		}

		// //////////////////////////////////////////////////////////////////////////////
		// Invoke user callback to get fresh output data UNLESS we are draining stream.
		// //////////////////////////////////////////////////////////////////////////////

		if (handle->drainCounter == 0) 
		{
			//RtAudioCallback callback = (RtAudioCallback)info->callback;
			auto callback = reinterpret_cast<CallbackPtr>(info->callback);

			double streamTime = getStreamTime();
			auto status = StreamIO_Good;

			if (stream.nDevPlayChannels > 0 && InternalAsioMgr::asioXRun == true)
			{
				status |= StreamIO_Output_Underflow;
				InternalAsioMgr::asioXRun = false;
			}

			if (stream.nDevRecChannels > 0 && InternalAsioMgr::asioXRun == true)
			{
				status |= StreamIO_Input_Overflow;
				InternalAsioMgr::asioXRun = false;
			}

			int cbReturnValue = callback(stream.userPlayBuffer, stream.userRecBuffer, stream.bufferSize, streamTime, status, info->userData);

			if (cbReturnValue == 2) 
			{
				stream.state = StreamState::STOPPING;
				handle->drainCounter = 2;

				// This function will be called by a spawned thread when the user
				// callback function signals that the stream should be stopped or
				// aborted.  It is necessary to handle it this way because the
				// callbackEvent() function must return before the ASIOStop()
				// function will return.

				unsigned threadId;
				//stream.callbackInfo.thread = _beginthreadex(NULL, 0, &asioStopStream, &stream.callbackInfo, 0, &threadId);

				_beginthreadex(NULL, 0, &asioStopStream, &stream.callbackInfo, 0, &threadId);

				// Much simpler now. @@ Except causes an exception!
				//std::thread bare([&] { this->stopStream(); });
				//std::thread bare([&] { asioStopStream(&stream.callbackInfo); });

			}
			else if (cbReturnValue == 1) 
			{
				handle->drainCounter = 1;
				handle->internalDrain = true;
			}
		}

		// /////////////////////////////////////////////////////////////////////////////
		// Let's take output data from internal form and put in device buffer
		// /////////////////////////////////////////////////////////////////////////////

		//if (stream.mode == OUTPUT || stream.mode == DUPLEX) 
		if (stream.nDevPlayChannels > 0)
		{
			unsigned nBytesPerSample = nBytes(stream.devPlayFormat);
			unsigned nBytesPerChannel = stream.bufferSize * nBytesPerSample;

			unsigned nChannels = stream.nDevPlayChannels + stream.nDevRecChannels;

			if (handle->drainCounter > 1) 
			{ 
				// write zeros to the output stream

				for (unsigned c = 0; c < nChannels; c++) 
				{
					if (handle->bufferInfos[c].isInput != ASIOTrue)
					{
						memset(handle->bufferInfos[c].buffers[bufferIndex], 0, nBytesPerChannel);
					}
				}

			}
			else if (stream.doConvertPlayBuffer) 
			{
#if 0
				// This currently assumes interleaving in the internal user buffer channels, and de-interleaves them
				convertBufferX(stream, stream.devPlayBuffer, stream.userPlayBuffer, stream.convertPlayInfo);
#else
				// Converts without changing the interleaving
				convertBuffer(stream, stream.devPlayBuffer, stream.userPlayBuffer, stream.convertPlayInfo);
#endif

#if 0
				if (stream.swapPlayBytes)
				{
					byteSwapBuffer(stream.devPlayFormat, stream.devPlayBuffer, stream.bufferSize * stream.nDevPlayChannels);
				}
#endif

				// Assumes interleaved here, so de-interleave. Also swap bytes if needed.

				for (unsigned c = 0, rel_c = 0; c < nChannels; c++) 
				{
					auto &cbi = handle->bufferInfos[c];

					if (cbi.isInput != ASIOTrue)
					{
						auto pChannelBytes = cbi.buffers[bufferIndex];
#if 0
						// This assumes already de-interleaved, and just moves to the asio buffer
						memcpy(pChannelBytes, &stream.devPlayBuffer[nBytesPerChannel * rel_c++], nBytesPerChannel);
#else
						// Swaps bytes too if necessary
						DeInterleaveChannel(stream.devPlayFormat, stream.devPlayBuffer, pChannelBytes, rel_c++, stream.nDevPlayChannels, stream.bufferSize, stream.swapPlayBytes);
#endif
					}
				}
			}
			else 
			{
				// Don't have to worry about interleaving or type conversion.
				// May have to swap bytes though.

				//if (stream.swapPlayBytes)
				//{
				//	byteSwapBuffer(stream.userFormat, stream.userPlayBuffer, stream.bufferSize * stream.nUserPlayChannels);
				//}

				for (unsigned c = 0, rel_c = 0; c < nChannels; c++) 
				{
					auto& cbi = handle->bufferInfos[c];

					if (cbi.isInput != ASIOTrue)
					{
						auto pChannelBytes = cbi.buffers[bufferIndex];
						auto src = &stream.userPlayBuffer[nBytesPerChannel * rel_c++];
						if (stream.swapPlayBytes)
						{
							CopyByteSwap(stream.devPlayFormat, pChannelBytes, src, stream.bufferSize, 1, 1);
						}
						else
						{
							memcpy(pChannelBytes, src, nBytesPerChannel);
						}
					}
				}
			}
		}

		// Don't bother draining input

		if (handle->drainCounter) 
		{
			handle->drainCounter++;
			goto unlock;
		}


	unlock:

		// The following call was suggested by Malte Clasen.  While the API
		// documentation indicates it should not be required, some device
		// drivers apparently do not function correctly without it.

		// @@ the example supplied by ASIO sdk does not include
		// the test. Why not?

		if (iam->driverData.postOutput)
		{
			iam->OutputReady();
		}

		tickStreamTime();

		return true;
	}


	bool AsioMgr::DisposeBuffers()
	{
		return iam->DisposeBuffers();
	}

	void AsioMgr::ConfigureUserCallback(CallbackPtr userCallback)
	{
		stream.callbackInfo.callback = userCallback;
	}

	unsigned long AsioMgr::SysRefTime()
	{
		return iam->SysRefTime();
	}

	long AsioMgr::SysMilliSeconds()
	{
		return iam->SysMilliSeconds();
	}

	long AsioMgr::SamplePos()
	{
		return iam->SamplePos();
	}

	void AsioMgr::GetTimeCode(long& hours, long& minutes, long& seconds, double& remainder)
	{
		return iam->GetTimeCode(hours, minutes, seconds, remainder);
	}

} // end of namespace