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

#include "DfxAudio.h"
#include "asiosys.h"
#include "asio.h"
#include "asiodrivers.h"
#include <string>
#include <vector>

//----------------------------------------------------------------------------------
// some external references coming from asiodrivers.cpp

extern AsioDrivers* asioDrivers;
extern bool loadAsioDriver(char* name);

//----------------------------------------------------------------------------------
// conversion from 64 bit ASIOSample/ASIOTimeStamp to double float
// from asio sdk

#if NATIVE_INT64
#define ASIO64toDouble(a)  (a)
#else
constexpr double twoRaisedTo32 = 4294967296.;
#define ASIO64toDouble(a)  ((a).lo + (a).hi * twoRaisedTo32)
#endif


namespace dfx
{
	extern unsigned long get_sys_reference_time();

	struct AsioHandle 
	{
		int drainCounter;       // Tracks callback counts when draining
		bool internalDrain;     // Indicates if stop is initiated from callback or not.
		ASIOBufferInfo* bufferInfos;
		HANDLE condition;

		AsioHandle() : drainCounter{}, internalDrain{}, bufferInfos{}, condition{}
		{
		}
	};


	// custom internal data storage

	//static constexpr long kMaxInputChannels = 32;
	//static constexpr long kMaxOutputChannels = 32;

	class DriverData {
	public:

		// From ASIOInit()

		ASIODriverInfo driverInfo{};

		// From ASIOGetChannels()

		long           nInputChannels{};
		long           nOutputChannels{};

		// From ASIOGetBufferSize()

		long           minSize{};
		long           maxSize{};
		long           preferredSize{};
		long           granularity{};

		// From ASIOGetSampleRate()

		ASIOSampleRate sampleRate{};

		// From ASIOOutputReady()

		bool           postOutput{};

		// From ASIOGetLatencies ()

		long           inputLatency{};
		long           outputLatency{};

		// From ASIOCreateBuffers ()

		// The bufferInfos and channelInfos arrays share the same indexing, as the data in them are linked together

		long nInputBuffers{};  // becomes number of actual created input buffers
		long nOutputBuffers{}; // becomes number of actual created output buffers

		ASIOBufferInfo* bufferInfos{}; //  [kMaxInputChannels + kMaxOutputChannels] ; // buffer info's

		// From ASIOGetChannelInfo()
		ASIOChannelInfo* channelInfos{}; // [kMaxInputChannels + kMaxOutputChannels] ; // channel info's

		// From ASIOGetSamplePosition()
		// data is converted to double floats for easier use, however 64 bit integer can be used, too

		double         nanoSeconds{};
		double         samplePos{};
		double         tcSamples{};	// time code samples

		// From bufferSwitchTimeInfo()

		ASIOTime       tInfo{};			// time info state
		unsigned long  sysRefTime{};      // system reference time, when bufferSwitch() was called

		// Signal the end of processing in this example
		bool           stopped{};

	public:

		DriverData() = default;
		virtual ~DriverData();

		bool Resize(int nInputChannels, int nOutputChannels);
	};


	// /////////////////////////////////////////////////////////////////////////////////
	// Internal Asio manager class
	// Because Asio only allows one driver loaded at a time, this is basically 
	// a static class

	class InternalAsioMgr {
	public:

		static DriverData driverData;
		static ASIOError lastResult;

	public:

		InternalAsioMgr();
		~InternalAsioMgr();

		long NumDevices();
		std::vector<std::string> DeviceNames();
		std::string DeviceName(long devId);

		static bool LoadDriver(const std::string& name);
		static bool InitDriver(bool verbose = true);
		static  void UnloadDriver();

		static long QueryDeviceID();

		static bool Start();
		static bool Stop();
		static bool Stopped();
		static bool Exit();

		void OutputReady();

		static bool QueryDriverInfo(bool verbose = true);
		static bool QueryDeviceInfo(DeviceInfo& devInfo, bool verbose = true);
		static bool CreateAsioBuffers(long nInputChannels_, long nOutputChannels_, long bufferSize, ASIOCallbacks *callbacks, bool verbose = true);
		static  bool DisposeBuffers();

		// Callbacks

		static ASIOTime* bufferSwitchTimeInfo(ASIOTime* timeInfo, long index, ASIOBool processNow);
		static void bufferSwitch(long index, ASIOBool processNow);
		static void sampleRateChanged(ASIOSampleRate sRate);
		static long asioMessage(long selector, long value, void* message, double* opt);

		// Other stuff

		static unsigned long SysRefTime();
		static long SysMilliSeconds();
		static long SamplePos();

		static void GetTimeCode(long& hours, long& minutes, long& seconds, double& remainder);
	};


	// Here's how you can link to it:

	static auto static_iam = std::make_shared<InternalAsioMgr>();


} // end of namespace