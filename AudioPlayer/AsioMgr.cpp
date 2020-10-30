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
#include <iostream>

namespace dfx
{
	std::string getAsioErrorString(ASIOError result)
	{
		switch (result)
		{
			case ASE_OK: return "OK";
			case ASE_SUCCESS: return "Future call success";
			case ASE_NotPresent: return "Hardware input or output is not present or available";
			case ASE_HWMalfunction: return "Hardware is malfunctioning";
			case ASE_InvalidParameter: return "Invalid input parameter";
			case ASE_InvalidMode: return "Hardware in bad mode or used in a bad mode";
			case ASE_SPNotAdvancing: return "Hardware is not running when sample position is inquired";
			case ASE_NoClock: return "Sample clock or rate cannot be determined or is not present";
			case ASE_NoMemory: return "Not enough memory to complete request";
			default: return "Unknown ASIO error";
		}
	}

	// ///////////////////////////

	std::string getAsioSampleTypeString(long t)
	{
		switch (t)
		{
			case ASIOSTInt16MSB: return "ASIOSTInt16MSB";
			case ASIOSTInt24MSB: return "ASIOSTInt24MSB";
			case ASIOSTInt32MSB: return "ASIOSTInt32MSB";
			case ASIOSTFloat32MSB: return "ASIOSTFloat32MSB";
			case ASIOSTFloat64MSB: return "ASIOSTFloat64MSB";

			case ASIOSTInt32MSB16: return "ASIOSTInt32MSB16";
			case ASIOSTInt32MSB18: return "ASIOSTInt32MSB18";
			case ASIOSTInt32MSB20: return "ASIOSTInt32MSB20";
			case ASIOSTInt32MSB24: return "ASIOSTInt32MSB24";

			case ASIOSTInt16LSB: return "ASIOSTInt16LSB";
			case ASIOSTInt24LSB: return "ASIOSTInt24LSB";
			case ASIOSTInt32LSB: return "ASIOSTInt32LSB";
			case ASIOSTFloat32LSB: return "ASIOSTFloat32LSB";
			case ASIOSTFloat64LSB: return "ASIOSTFloat64LSB";

			case ASIOSTInt32LSB16: return "ASIOSTInt32LSB16";
			case ASIOSTInt32LSB18: return "ASIOSTInt32LSB18";
			case ASIOSTInt32LSB20: return "ASIOSTInt32LSB20";
			case ASIOSTInt32LSB24: return "ASIOSTInt32LSB24";

			case ASIOSTDSDInt8LSB1: return "ASIOSTDSDInt8LSB1";
			case ASIOSTDSDInt8MSB1: return "ASIOSTDSDInt8MSB1";
			case ASIOSTDSDInt8NER8: return "ASIOSTDSDInt8NER8";

			default: return "Unknown sample type";
		}
	}


	// ///////////////////////////

	unsigned long get_sys_reference_time()
	{
#if WINDOWS
		// @@ WARNING: YOU NEED winmm.lib in the linker for this to work
		return timeGetTime();
#elif MAC
		static constexpr double twoRaisedTo32 = 4294967296.;
		UnsignedWide ys;
		Microseconds(&ys);
		double r = ((double)ys.hi * twoRaisedTo32 + (double)ys.lo);
		return (unsigned long)(r / 1000.);
#endif
	}


	//----------------------------------------------------------------------------------

	// static class variables

	DriverData AsioMgr::driverData{};
	ASIOError AsioMgr::lastResult{};
	bool AsioMgr::asioXRun{};

	//----------------------------------------------------------------------------------

	// //////////////////////////////////////////////////
	// Driver data stuff

	DriverData::~DriverData()
	{
		// ASSUMS ASIO driver is in stopped state

		delete[] bufferInfos;
		delete[] channelInfos;
	}


	bool DriverData::Resize(int nInputChannels_, int nOutputChannels_)
	{
		// ASSUMES ASIO driver is in stopped state

		delete[] bufferInfos;
		delete[] channelInfos;

		bufferInfos = new ASIOBufferInfo[nInputChannels_ + nOutputChannels_];

		if (!bufferInfos)
		{
			// out of memory
			return false;
		}

		channelInfos = new ASIOChannelInfo[nInputChannels_ + nOutputChannels_];

		if (!channelInfos)
		{
			delete[] bufferInfos;
			bufferInfos = 0;
			return false;
		}

		nInputChannels = nInputChannels_;
		nOutputChannels = nOutputChannels_;

		return true;
	}

	// ////////////////////////////////////////////////////////////////////////
	// 
	// 
	// ////////////////////////////////////////////////////////////////////////

	static int fox = 0;  // Just for debugging

	AsioMgr::AsioMgr()
	: DfxAudio()
	{
		// This is what asiodrivers.cpp does if it tries
		// to use this pointer and it's null:

		if (!asioDrivers)
		{
			asioDrivers = new AsioDrivers();
		}

		++fox;
	}

	AsioMgr::~AsioMgr()
	{
		Close(); // Removes current driver too
		//asioDrivers->removeCurrentDriver();
		delete asioDrivers;
		asioDrivers = 0;
		--fox;
	}

	int AsioMgr::LastError()
	{
		// TBD
		return 0;
	}

	long AsioMgr::NumDevices()
	{
		return asioDrivers->asioGetNumDev();
	}

	std::vector<std::string> AsioMgr::DeviceNames()
	{
		auto nDevices = NumDevices();
		std::vector<std::string> names(nDevices);

		for (long i = 0; i < nDevices; i++)
		{
			names[i] = DeviceName(i);
		}

		return names;
	}

	std::string AsioMgr::DeviceName(long devId)
	{
		constexpr int name_size = MAXDRVNAMELEN;
		char driverName[name_size];

		driverName[0] = 0;

		auto nDevices = NumDevices();

		if (devId < 0 || devId >= nDevices)
		{
			// @@ TODO:

			//std::stringstream s;
			//s << "AsioMgr::DeviceName(): device ID " << devId << " out of range.";
			//HandleError(s);
		}

		ASIOError result = asioDrivers->asioGetDriverName(devId, driverName, name_size);

		if (result == DRVERR_DEVICE_NOT_FOUND)
		{
			// @@ TODO:
			//std::stringstream s;
			//s << "AsioMgr::DeviceNames(): asio device " << devId << " not found";
			//HandleError(s);
		}

		return driverName;
	}



	bool AsioMgr::LoadDriver(const std::string& name)
	{
		auto b = loadAsioDriver(const_cast<char*>(name.c_str()));
		return b;
	}

	bool AsioMgr::InitDriver(bool verbose)
	{
		// Per SDK document, do this so asioMessage callback works
		driverData.driverInfo.asioVersion = 2;

		// So driver popup works
		// GetDesktopWindow() might work better here. See
		// DirectSound comment in RtAudio.cpp
		driverData.driverInfo.sysRef = GetForegroundWindow();

		lastResult = ASIOInit(&driverData.driverInfo);

		if (lastResult == ASE_OK)
		{
			if (verbose)
			{
				std::cout << "asioVersion:   " << driverData.driverInfo.asioVersion << std::endl;
				std::cout << "driverVersion: " << driverData.driverInfo.driverVersion << std::endl;
				std::cout << "Name:          " << driverData.driverInfo.name << std::endl;
				std::cout << "Status:        " << driverData.driverInfo.errorMessage << std::endl;
			}

			bool  b = QueryDriverInfo(verbose);

			if (b)
			{
				b = QueryDeviceInfo(devInfo, verbose);
			}

			return b;
		}
		else return false;
	}

	void AsioMgr::UnloadDriver()
	{
		asioDrivers->removeCurrentDriver();
	}


	long AsioMgr::QueryDeviceID()
	{
		return asioDrivers->getCurrentDriverIndex();
	}


	bool AsioMgr::PopupControlPanel()
	{
		auto rv = ASIOControlPanel();
		return rv == ASE_OK;
	}

	//----------------------------------------------------------------------------------

	bool AsioMgr::QueryDriverInfo(bool verbose)
	{
		// Collect informational data for the driver.
		// Get the number of available channels

		lastResult = ASIOGetChannels(&driverData.nInputChannels, &driverData.nOutputChannels);

		if (lastResult != ASE_OK)
		{
			return false;
		}

		if (verbose)
		{
			std::cout << "ASIOGetChannels (num inputs: " << driverData.nInputChannels << ", num outputs: " << driverData.nOutputChannels << ")" << std::endl;
		}

		// get the usable buffer sizes

		lastResult = ASIOGetBufferSize(&driverData.minSize, &driverData.maxSize, &driverData.preferredSize, &driverData.granularity);

		if (lastResult != ASE_OK)
		{
			return false;
		}

		if (verbose)
		{
			std::cout << "ASIOGetBufferSize: (min: " << driverData.minSize << ", max: " << driverData.maxSize;
			std::cout << ", preferred: " << driverData.preferredSize << ", granularity: " << driverData.granularity << ")";
			std::cout << std::endl;
		}

		// get the currently selected sample rate

		lastResult = ASIOGetSampleRate(&driverData.sampleRate);

		if (lastResult != ASE_OK)
		{
			return false;
		}

		if (verbose)
		{
			std::cout << "ASIOGetSampleRate (samplerate: " << driverData.sampleRate << ")" << std::endl;
		}

		if (driverData.sampleRate <= 0.0 || driverData.sampleRate > 192000.0)
		{
			// Driver does not store it's internal sample rate, so set it to a known one.
			// Usually you should check beforehand that the selected sample rate is valid
			// with ASIOCanSampleRate().

			lastResult = ASIOSetSampleRate(44100.0);

			if (lastResult == ASE_OK)
			{
				lastResult = ASIOGetSampleRate(&driverData.sampleRate);
				if (lastResult == ASE_OK)
				{
					if (verbose)
					{
						std::cout << "ASIOGetSampleRate: (sampleRate: " << driverData.sampleRate << ")" << std::endl;
					}
				}
				else
				{
					//rv = -6;
					return false;
				}
			}
			else
			{
				return false;
			}
		}

		// check wether the driver requires the ASIOOutputReady() optimization
		// (can be used by the driver to reduce output latency by one block)

		auto rv = ASIOOutputReady();

		if (rv == ASE_OK)
		{
			driverData.postOutput = true;
		}
		else
		{
			driverData.postOutput = false;
		}

		if (verbose)
		{
			std::cout << "ASIOOutputReady(): " << (driverData.postOutput ? "Supported" : "Not supported") << std::endl;
		}

		return lastResult == ASE_OK;
	}

	bool AsioMgr::QueryDeviceInfo(DeviceInfo& devInfo, bool verbose)
	{
		// ASSUMES QueryDeviceInfo has already been called
		// But don't call after creating buffers, cause driverData num channels
		// will have possibly changed.

		devInfo.name = driverData.driverInfo.name;
		devInfo.devID = QueryDeviceID();

		devInfo.nInChannelsAvail = driverData.nInputChannels;
		devInfo.nOutChannelsAvail = driverData.nOutputChannels;

		if (devInfo.nOutChannelsAvail > 0 && devInfo.nInChannelsAvail > 0)
		{
			devInfo.nDuplexChannelsAvail = (devInfo.nInChannelsAvail < devInfo.nOutChannelsAvail) ? devInfo.nInChannelsAvail : devInfo.nOutChannelsAvail;
		}

		// Get channel names for input channels
		// Also get supported device sample type

		ASIOChannelInfo channelInfo;
		ASIOSampleType stype{ ASIOSTLastEntry };

		for (long i = 0; i < devInfo.nInChannelsAvail; i++)
		{
			channelInfo.channel = i;
			channelInfo.isInput = true;

			lastResult = ASIOGetChannelInfo(&channelInfo);

			if (lastResult != ASE_OK)
			{
				//asioDrivers->removeCurrentDriver();
				//std::stringstream s;
				//s << "AsioMgr::FillDeviceInfo(): (" << getAsioErrorString(result) << ") getting driver channel info for (" << info.name << ").";
				//HandleError(s);
				//return info; // Will never get here
				return false;
			}

			if (i == 0)
			{
				// We'll use the results from the first input channel to tell us what type we are.
				stype = channelInfo.type;
			}

			devInfo.inputNames.push_back(channelInfo.name);
		}

		// Get channel names for output channels

		for (long i = 0; i < devInfo.nOutChannelsAvail; i++)
		{
			channelInfo.channel = i;
			channelInfo.isInput = false;

			lastResult = ASIOGetChannelInfo(&channelInfo);

			if (lastResult != ASE_OK)
			{
				//asioDrivers->removeCurrentDriver();
				//std::stringstream s;
				//s << "AsioMgr::FillDeviceInfo(): (" << getAsioErrorString(result) << ") getting driver channel info for (" << info.name << ").";
				//HandleError(s);
				//return info; // Will never get here
				return false;
			}

			if (i == 0 && stype == ASIOSTLastEntry)
			{
				// We'll use the results from the first output channel to tell us what type we are.
				stype = channelInfo.type;
			}


			devInfo.outputNames.push_back(channelInfo.name);
		}

		if (verbose)
		{
			std::cout << "Channel Input Names:" << std::endl;

			for (size_t i = 0; i < devInfo.inputNames.size(); i++)
			{
				std::cout << i << ": " << devInfo.inputNames[i] << std::endl;
			}

			std::cout << std::endl;

			std::cout << "Channel Output Names:" << std::endl;

			for (size_t i = 0; i < devInfo.outputNames.size(); i++)
			{
				std::cout << i << ": " << devInfo.outputNames[i] << std::endl;
			}
		}

		// Determine supported data types ... just check first input channel and assume rest are the same.

		switch (stype)
		{
			case ASIOSTInt16MSB:
			{
				devInfo.format = SampleFormat::SINT16;
				devInfo.little_endian = false;
			}
			break;

			case ASIOSTInt16LSB:
			{
				devInfo.format = SampleFormat::SINT16;
				devInfo.little_endian = true;
			}
			break;

			case ASIOSTInt32MSB:
			{
				devInfo.format = SampleFormat::SINT32;
				devInfo.little_endian = false;
			}
			break;

			case ASIOSTInt32LSB:
			{
				devInfo.format = SampleFormat::SINT32;
				devInfo.little_endian = true;
			}
			break;

			case ASIOSTFloat32MSB:
			{
				devInfo.format = SampleFormat::FLOAT32;
				devInfo.little_endian = false;
			}
			break;

			case ASIOSTFloat32LSB:
			{
				devInfo.format = SampleFormat::FLOAT32;
				devInfo.little_endian = true;
			}
			break;

			case ASIOSTFloat64MSB:
			{
				devInfo.format = SampleFormat::FLOAT64;
				devInfo.little_endian = false;
			}
			break;

			case ASIOSTFloat64LSB:
			{
				devInfo.format = SampleFormat::FLOAT64;
				devInfo.little_endian = true;
			}
			break;

			case ASIOSTInt24MSB:
			{
				devInfo.format = SampleFormat::SINT24;
				devInfo.little_endian = false;
			}
			break;

			case ASIOSTInt24LSB:
			{
				devInfo.format = SampleFormat::SINT24;
				devInfo.little_endian = true;
			}
			break;

			default:
			{
				//asioDrivers->removeCurrentDriver();
				//std::stringstream s;
				//s << "AsioMgr::FillDeviceInfo(): (" << "Invalid sample type: " << stype << " for (" << info.name << ").";
				//HandleError(s);
				//return info; // Will never get here
				return false;
			}
		}

		if (verbose)
		{
			std::cout << std::endl;
			std::cout << "Device sample type: " << getAsioSampleTypeString(stype) << " - " << (devInfo.little_endian ? "little endian" : "big endian");
			std::cout << std::endl;
		}

		// What sample rates are supported

		devInfo.supportedSampleRates.clear();

		for (unsigned int i = 0; i < MAX_SAMPLE_RATES; i++)
		{
			auto rv = ASIOCanSampleRate((ASIOSampleRate)SAMPLE_RATES[i]);
			if (rv == ASE_OK)
			{
				devInfo.supportedSampleRates.push_back(SAMPLE_RATES[i]);

				if (!devInfo.preferredSampleRate || (SAMPLE_RATES[i] <= 48000 && SAMPLE_RATES[i] > devInfo.preferredSampleRate))
				{
					devInfo.preferredSampleRate = SAMPLE_RATES[i];
				}
			}
		}

		if (verbose)
		{
			std::cout << std::endl;

			std::cout << "Supported sample rates:" << std::endl;

			for (size_t i = 0; i < devInfo.supportedSampleRates.size(); i++)
			{
				std::cout << devInfo.supportedSampleRates[i] << std::endl;
			}

			std::cout << std::endl;
		}

		devInfo.valid = true;

		return true;
	}

	//----------------------------------------------------------------------------------

	bool AsioMgr::CreateAsioBuffers(long nInputChannels_, long nOutputChannels_, long bufferSize, ASIOCallbacks* callbacks, bool verbose)
	{
		// Create buffers for all inputs and outputs of the audio device with
		// the preferredSize from ASIOGetBufferSize() as the buffer size.

		// IMPLIES WE ARE STOPPED!

		Stop(); // @@ TODO: This probably isn't enough, 'cause has to wind down.

		bool b = driverData.Resize(nInputChannels_, nOutputChannels_);

		if (!b)
		{
			// @@ TODO: Memory error!
			return b;
		}

		// fill the bufferInfos from the start without a gap

		// Point to first buffer info

		auto info = driverData.bufferInfos;

		// prepare inputs (Though this is not necessaily required, no opened inputs will work, too)

		driverData.nInputBuffers = driverData.nInputChannels;

		// Walk thru all input buffer infos

		for (long i = 0; i < driverData.nInputBuffers; i++, info++)
		{
			info->isInput = ASIOTrue;
			info->channelNum = i;
			info->buffers[0] = 0;
			info->buffers[1] = 0;
		}

		// prepare outputs

		driverData.nOutputBuffers = driverData.nOutputChannels;

		// Walk thru all out buffer infos

		for (long i = 0; i < driverData.nOutputBuffers; i++, info++)
		{
			info->isInput = ASIOFalse;
			info->channelNum = i;
			info->buffers[0] = 0;
			info->buffers[1] = 0;
		}

		// create and activate buffers

		lastResult = ASIOCreateBuffers(driverData.bufferInfos, driverData.nInputBuffers + driverData.nOutputBuffers, bufferSize, callbacks);

		if (lastResult == ASE_OK)
		{
			// now get all the buffer details, sample word length, name, word clock group and activation

			for (long i = 0; i < driverData.nInputBuffers + driverData.nOutputBuffers; i++)
			{
				auto& ci = driverData.channelInfos[i];
				auto& bi = driverData.bufferInfos[i];

				ci.channel = bi.channelNum;
				ci.isInput = bi.isInput;

				lastResult = ASIOGetChannelInfo(&ci);

				if (lastResult == ASE_OK)
				{
					if (verbose)
					{
						std::cout << "Channel " << ci.name << ":";
						std::cout << " num = " << ci.channel;
						std::cout << " isActive = " << ci.isActive;
						std::cout << " group = " << ci.channelGroup;
						std::cout << " type = " << ci.type;
						std::cout << std::endl;
					}
				}
				else
				{
					break;
				}
			}

			if (lastResult == ASE_OK)
			{
				// Get the input and output latencies
				// Latencies often are only valid after ASIOCreateBuffers()
				// (input latency is the age of the first sample in the currently returned audio block)
				// (output latency is the time the first sample in the currently returned audio block requires to get to the output)

				lastResult = ASIOGetLatencies(&driverData.inputLatency, &driverData.outputLatency);

				if (lastResult == ASE_OK)
				{
					if (verbose)
					{
						std::cout << "ASIOGetLatencies (input: " << driverData.inputLatency << ", output: " << driverData.outputLatency << ");" << std::endl;
					}
				}
			}
		}

		return lastResult == ASE_OK;
	}


	bool AsioMgr::DisposeBuffers()
	{
		ASIODisposeBuffers();
		delete[] driverData.bufferInfos;
		driverData.bufferInfos = 0;
		delete[] driverData.channelInfos;
		driverData.channelInfos = 0;
		return true;
	}


	// ---------------------------------------


	bool AsioMgr::Start()
	{
		lastResult = ASIOStart();

		if (lastResult == ASE_OK)
		{
			stream.state = StreamState::RUNNING;
			return true;
		}
		else return false;
	}

	bool AsioMgr::Stop()
	{
		lastResult = ASIOStop();

		if (lastResult == ASE_OK)
		{
			stopStream();
			return true;
		}
		else return false;
	}

	bool AsioMgr::Stopped()
	{
		// @@ TODO: HMMM...
		//return InternalAsioMgr::driverData.stopped;
		return stream.state == StreamState::STOPPED;
	}

	bool AsioMgr::Close()
	{
		closeStream();

		DisposeBuffers();
		lastResult = ASIOExit(); // Removes current driver

		return lastResult == ASE_OK;
	}

	void AsioMgr::OutputReady()
	{
		ASIOOutputReady();
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

		//UnloadDriver();

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
		}

		stream.apiHandle = 0;

		DfxAudio::closeStream(); // Gets rid of user-controlled stream buffers
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
		asioXRun = false;

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

		bool result =Stop();

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
	// function will return. And that of course causes other issues
	// as outline below where we must detach the thread after stopping
	// the stream.

	static unsigned __stdcall asioStopStream(void* ptr)
	{
		CallbackInfo* info = (CallbackInfo*)ptr;
		auto object = (AsioMgr*)info->object;

		object->stopStream();

		// Let the thread run die on its own. We have to 
		// do this, because it must live longer than this
		// function (asioStopStream) since in fact the thread
		// is the one that called this function. So if we 
		// don't detach, amusement ensues.

		info->thread.detach();

		return 0;
	}


	//
	///////////////////////////////////////////////////////////

	static ASIOCallbacks asioCallbacks;      // This has to hang around while stream is running
	static CallbackInfo *asioCallbackInfo;

	void AsioMgr::bufferSwitch(long index, long processNow)
	{
		// Static member function
		// Note: long processNow is really ASIOBool processNow

		// asioCallbackInfo must be static
		auto object = reinterpret_cast<AsioMgr*>(asioCallbackInfo->object);

		// @@ HACK
		// get the system reference time
		object->driverData.sysRefTime = get_sys_reference_time();

		object->callbackEvent(index);
	}

	void AsioMgr::sampleRateChanged(double sRate)
	{
		// Static member function
		// Note: double sRate is really ASIOSampleRate srate

		// Callback

		// @@ TODO

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

		// Callback for messages from the asio driver

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

				driverData.stopped;  // In this sample the processing will just stop
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

				asioXRun = true;
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
			{
				asioXRun = true;
				return 0L; // Return value actually ignored.
			}
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

		driverData.sampleRate = sampleRate_; // @@ Should we? To keep things consistent?

		stream.sampleRate = sampleRate_;

		stream.bufferSize = bufferSize_; // @@ TODO: Check compatibility

		stream.callbackInfo.object = this;
		asioCallbackInfo = &stream.callbackInfo; // Set static pointer
		stream.callbackInfo.userData = userData_;

		//

		asioCallbacks.asioMessage = asioMessage;
		asioCallbacks.sampleRateDidChange = sampleRateChanged;
		asioCallbacks.bufferSwitchTimeInfo = nullptr; // bufferSwitchTimeInfo; //  nullptr;
		asioCallbacks.bufferSwitch = bufferSwitch;

		bool b = CreateAsioBuffers(nInputChannels_, nOutputChannels_, stream.bufferSize, &asioCallbacks, verbose);

		if (!b)
		{
			return false;
		}

		// Only do these after opening, so we have right number of channels

		stream.nUserPlayChannels = driverData.nOutputChannels;
		stream.nUserRecChannels = driverData.nInputChannels;

		stream.nDevPlayChannels = driverData.nOutputChannels;
		stream.nDevRecChannels = driverData.nInputChannels;

		// Only valid after creating the asio buffers

		stream.playLatency = driverData.outputLatency;
		stream.recLatency = driverData.inputLatency;

		// ////////////////////////////

		stream.devPlayID = QueryDeviceID();
		stream.devRecID = QueryDeviceID();

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
				handle->bufferInfos = driverData.bufferInfos;

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

				// We spawn a thread and move it into the CallbackInfo structure.
				// That way it hangs around long enough to do its thing.

				info->thread = std::thread([&]{ this->stopStream(); }); // @@ Note: move assigned.
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

			if (stream.nDevPlayChannels > 0 && asioXRun == true)
			{
				status |= StreamIO_Output_Underflow;
				asioXRun = false;
			}

			if (stream.nDevRecChannels > 0 && AsioMgr::asioXRun == true)
			{
				status |= StreamIO_Input_Overflow;
				asioXRun = false;
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

				// We spawn a thread and move it into the CallbackInfo structure.
				// That way it hangs around long enough to do its thing.

				info->thread = std::thread([&] { asioStopStream(&stream.callbackInfo); }); // @@ Note: move assigned.

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

		if (driverData.postOutput)
		{
			OutputReady();
		}

		tickStreamTime();

		return true;
	}

	void AsioMgr::ConfigureUserCallback(CallbackPtr userCallback)
	{
		stream.callbackInfo.callback = userCallback;
	}

	unsigned long AsioMgr::SysRefTime()
	{
		return driverData.sysRefTime;
	}

	long AsioMgr::SysMilliSeconds()
	{
		return (long)(driverData.nanoSeconds / 1000000.0);
	}

	long AsioMgr::SamplePos()
	{
		return (long)driverData.samplePos;
	}

	void AsioMgr::GetTimeCode(long& hours, long& minutes, long& seconds, double& remainder)
	{
		auto& dd = driverData;
		auto& sampleRate = dd.sampleRate;

		remainder = dd.tcSamples;
		hours = (long)(remainder / (sampleRate * 3600));
		remainder -= hours * sampleRate * 3600;
		minutes = (long)(remainder / (sampleRate * 60));
		remainder -= minutes * sampleRate * 60;
		seconds = (long)(remainder / sampleRate);
		remainder -= seconds * sampleRate;
	}

} // end of namespace