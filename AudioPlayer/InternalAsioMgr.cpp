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

#include "InternalAsioMgr.h"
#include <iostream>
#include <sstream>

//////////////////////////////////////////////////////////////////

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

	DriverData InternalAsioMgr::driverData{};
	ASIOError InternalAsioMgr::lastResult{};
	bool InternalAsioMgr::asioXRun{};

	std::shared_ptr<InternalAsioMgr> static_iam = std::make_shared<InternalAsioMgr>();

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
	// A static class
	//

	static int fox = 0;

	InternalAsioMgr::InternalAsioMgr()
	{
		// This is what asiodrivers.cpp does if it tries
		// to use this pointer and it's null:

		if (!asioDrivers)
		{
			asioDrivers = new AsioDrivers();
		}

		++fox;
	}

	InternalAsioMgr::~InternalAsioMgr()
	{
		Exit(); // Removes current driver too
		//asioDrivers->removeCurrentDriver();
		delete asioDrivers;
		asioDrivers = 0;
		--fox;
	}


	long InternalAsioMgr::NumDevices()
	{
		return asioDrivers->asioGetNumDev();
	}

	std::vector<std::string> InternalAsioMgr::DeviceNames()
	{
		auto nDevices = NumDevices();
		std::vector<std::string> names(nDevices);

		for (long i = 0; i < nDevices; i++)
		{
			names[i] = DeviceName(i);
		}

		return names;
	}

	std::string InternalAsioMgr::DeviceName(long devId)
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



	bool InternalAsioMgr::LoadDriver(const std::string& name)
	{
		auto b = loadAsioDriver(const_cast<char*>(name.c_str()));
		return b;
	}


	bool InternalAsioMgr::InitDriver(bool verbose)
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
			return true;
		}
		else return false;
	}

	void InternalAsioMgr::UnloadDriver()
	{
		asioDrivers->removeCurrentDriver();
	}


	long InternalAsioMgr::QueryDeviceID()
	{
		return asioDrivers->getCurrentDriverIndex();
	}

	bool InternalAsioMgr::PopupControlPanel()
	{
		auto rv = ASIOControlPanel();
		return rv == ASE_OK;
	}

	//----------------------------------------------------------------------------------

	bool InternalAsioMgr::QueryDriverInfo(bool verbose)
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
			std::cout << "ASIOGetBUfferSize: (min: " << driverData.minSize << ", max: " << driverData.maxSize;
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

	bool InternalAsioMgr::QueryDeviceInfo(DeviceInfo& devInfo, bool verbose)
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

	bool InternalAsioMgr::CreateAsioBuffers(long nInputChannels_, long nOutputChannels_, long bufferSize, ASIOCallbacks *callbacks, bool verbose)
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

		lastResult = ASIOCreateBuffers(driverData.bufferInfos,	driverData.nInputBuffers + driverData.nOutputBuffers, bufferSize, callbacks);

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


	bool InternalAsioMgr::DisposeBuffers()
	{
		ASIODisposeBuffers();
		delete[] driverData.bufferInfos;
		driverData.bufferInfos = 0;
		delete[] driverData.channelInfos;
		driverData.channelInfos = 0;
		return true;
	}


	// ---------------------------------------

	bool InternalAsioMgr::Start()
	{
		lastResult = ASIOStart();
		return lastResult == ASE_OK;
	}


	bool InternalAsioMgr::Stop()
	{
		lastResult = ASIOStop();
		return lastResult == ASE_OK;
	}

	bool InternalAsioMgr::Stopped()
	{
		return InternalAsioMgr::driverData.stopped;
	}

	bool InternalAsioMgr::Exit()
	{
		DisposeBuffers();
		lastResult = ASIOExit(); // Removes current driver
		return lastResult;
	}

	void InternalAsioMgr::OutputReady()
	{
		ASIOOutputReady();
	}

	ASIOTime* InternalAsioMgr::bufferSwitchTimeInfo(ASIOTime* timeInfo, long index, ASIOBool processNow)
	{
		// Static member function

		// The actual processing callback.
		// Beware that this is normally in a separate thread, hence be sure that you take care
		// about thread synchronization. This is omitted here for simplicity.

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

		// buffer size in samples
		long buffSize = driverData.preferredSize;

		// perform the processing
		for (int i = 0; i < driverData.nInputBuffers + driverData.nOutputBuffers; i++)
		{
			if (driverData.bufferInfos[i].isInput == false) // that is, outputs only
			{
				// OK do processing for the outputs only
				switch (driverData.channelInfos[i].type)
				{
					case ASIOSTInt16LSB:
						memset(driverData.bufferInfos[i].buffers[index], 0, buffSize * sizeof(int16_t));
					break;
					case ASIOSTInt24LSB:		// used for 20 bits as well
						memset(driverData.bufferInfos[i].buffers[index], 0, buffSize * sizeof(int24_t));
					break;
					case ASIOSTInt32LSB:
						memset(driverData.bufferInfos[i].buffers[index], 0, buffSize * sizeof(int32_t));
					break;
					case ASIOSTFloat32LSB:		// IEEE 754 32 bit float, as found on Intel x86 architecture
						memset(driverData.bufferInfos[i].buffers[index], 0, buffSize * sizeof(float));
					break;
						case ASIOSTFloat64LSB: 		// IEEE 754 64 bit double float, as found on Intel x86 architecture
						memset(driverData.bufferInfos[i].buffers[index], 0, buffSize * sizeof(double));
					break;

					// These are used for 32 bit data buffers, with different alignment of the data inside.
					// 32 bit PCI bus systems can more easily be used with these.
					case ASIOSTInt32LSB16:		// 32 bit data with 18 bit alignment
					case ASIOSTInt32LSB18:		// 32 bit data with 18 bit alignment
					case ASIOSTInt32LSB20:		// 32 bit data with 20 bit alignment
					case ASIOSTInt32LSB24:		// 32 bit data with 24 bit alignment
						memset(driverData.bufferInfos[i].buffers[index], 0, buffSize * sizeof(int32_t));
					break;

					case ASIOSTInt16MSB:
						memset(driverData.bufferInfos[i].buffers[index], 0, buffSize * sizeof(int32_t));
					break;
					case ASIOSTInt24MSB:		// used for 20 bits as well
						memset(driverData.bufferInfos[i].buffers[index], 0, buffSize * sizeof(int24_t));
					break;
					case ASIOSTInt32MSB:
						memset(driverData.bufferInfos[i].buffers[index], 0, buffSize * sizeof(int32_t));
					break;
					case ASIOSTFloat32MSB:		// IEEE 754 32 bit float, as found on Intel x86 architecture
						memset(driverData.bufferInfos[i].buffers[index], 0, buffSize * sizeof(float));
					break;
					case ASIOSTFloat64MSB: 		// IEEE 754 64 bit double float, as found on Intel x86 architecture
						memset(driverData.bufferInfos[i].buffers[index], 0, buffSize * sizeof(double));
					break;

					// These are used for 32 bit data buffers, with different alignment of the data inside.
					// 32 bit PCI bus systems can be more easily used with these.
					case ASIOSTInt32MSB16:		// 32 bit data with 18 bit alignment
					case ASIOSTInt32MSB18:		// 32 bit data with 18 bit alignment
					case ASIOSTInt32MSB20:		// 32 bit data with 20 bit alignment
					case ASIOSTInt32MSB24:		// 32 bit data with 24 bit alignment
						memset(driverData.bufferInfos[i].buffers[index], 0, buffSize * sizeof(int32_t));
					break;
				}
			}
		}

		// If the driver supports the ASIOOutputReady() optimization, do it here, all data is ready

		if (driverData.postOutput)
		{
			ASIOOutputReady();
		}

		processedSamples += buffSize;

		return 0L;
	}

	//----------------------------------------------------------------------------------

	void InternalAsioMgr::bufferSwitch(long index, ASIOBool processNow)
	{
		// Static member function

		// The actual processing callback.
		// Beware that this is normally in a separate thread, hence be sure that you take care
		// about thread synchronization. This is omitted here for simplicity.

		// As this is a "back door" into the bufferSwitchTimeInfo a timeInfo needs to be created
		// though it will only set the timeInfo.samplePosition and timeInfo.systemTime fields and the according flags

		ASIOTime  timeInfo;
		memset(&timeInfo, 0, sizeof(timeInfo));

		// Get the time stamp of the buffer, not necessary if no
		// synchronization to other media is required

		if (ASIOGetSamplePosition(&timeInfo.timeInfo.samplePosition, &timeInfo.timeInfo.systemTime) == ASE_OK)
		{
			timeInfo.timeInfo.flags = kSystemTimeValid | kSamplePositionValid;
		}

		bufferSwitchTimeInfo(&timeInfo, index, processNow);
	}


	//----------------------------------------------------------------------------------

	void InternalAsioMgr::sampleRateChanged(ASIOSampleRate sRate)
	{
		// Static member function

		// Callback

		// Do whatever you need to do if the sample rate changed
		// usually this only happens during external sync.
		// Audio processing is not stopped by the driver, actual sample rate
		// might not have even changed, maybe only the sample rate status of an
		// AES/EBU or S/PDIF digital input at the audio device.
		// You might have to update time/sample related conversion routines, etc.
	}

	//----------------------------------------------------------------------------------

	long InternalAsioMgr::asioMessage(long selector, long value, void* message, double* opt)
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

			default:
			{
				throw std::exception("asioMessages(): Invalid switch statement selector");
			}
		}

		return ret;
	}

	// /////////////////////////////////////


	unsigned long InternalAsioMgr::SysRefTime()
	{
		return driverData.sysRefTime;
	}

	long InternalAsioMgr::SysMilliSeconds()
	{
		return (long)(driverData.nanoSeconds / 1000000.0);
	}

	long InternalAsioMgr::SamplePos()
	{
		return (long)driverData.samplePos;
	}

	void InternalAsioMgr::GetTimeCode(long& hours, long& minutes, long& seconds, double& remainder)
	{
		auto& dd = InternalAsioMgr::driverData;
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