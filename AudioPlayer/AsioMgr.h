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

	// ------

	struct AsioHandle
	{
		int drainCounter;            // Tracks callback counts when draining
		bool internalDrain;          // Indicates if stop is initiated from callback or not.
		ASIOBufferInfo* bufferInfos; // This struct does *not* own these
		HANDLE condition;

		AsioHandle() : drainCounter{}, internalDrain{}, bufferInfos{}, condition{}
		{
		}
	};


	// ---- custom internal data storage

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


	// /////////////////////////////////////////////////////////////////////////////
	// 
	// /////////////////////////////////////////////////////////////////////////////


	class AsioMgr : public DfxAudio {
	public:

		static DriverData driverData;
		static ASIOError lastResult;

		static bool asioXRun;

	public:

		AsioMgr();
		virtual ~AsioMgr();

	public:

		virtual bool LoadDriver(const std::string& name);
		virtual bool InitDriver(bool verbose = true);
		virtual void UnloadDriver();

		virtual long NumDevices();
		virtual std::vector<std::string> DeviceNames();
		virtual std::string DeviceName(long devId);
		virtual long QueryDeviceID();

		virtual int LastError();

	public:

		// virtual void verifyStream();

		virtual void closeStream(); // should be overidden
		virtual void startStream();
		virtual void stopStream();
		virtual void abortStream();

	public:

		virtual bool Start();
		virtual bool Stop();
		virtual bool Stopped();

		virtual void ConfigureUserCallback(CallbackPtr userCallback);

		virtual bool Open(long nInputChannels_, long nOutputChannels_, long bufferSize_, unsigned sampleRate_, void *userData_, bool verbose);
		virtual bool Close();

	public:

		bool QueryDriverInfo(bool verbose = true);
		bool QueryDeviceInfo(DeviceInfo& devInfo, bool verbose = true);
		bool CreateAsioBuffers(long nInputChannels_, long nOutputChannels_, long bufferSize, ASIOCallbacks* callbacks, bool verbose = true);
		bool DisposeBuffers();

		bool PopupControlPanel();

		// ///
		// Our low level call backs
		static void bufferSwitch(long index, long processNow);
		static void sampleRateChanged(double sRate);
		static long asioMessage(long selector, long value, void* message, double* opt);

		// ///

		void OutputReady();

		bool callbackEvent(long bufferIndex);

		static unsigned long SysRefTime();
		static long SysMilliSeconds();
		static long SamplePos();

		static void GetTimeCode(long& hours, long& minutes, long& seconds, double& remainder);

	};

} // end of namespace