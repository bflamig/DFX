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
#include <string>

namespace dfx
{
	// /////////////////////////////////////////////////////////////////////////////
	// 
	// /////////////////////////////////////////////////////////////////////////////


	class InternalAsioMgr; // forward decl

	class AsioMgr : public DfxAudio {
	public:

		std::shared_ptr<InternalAsioMgr> iam; // A static singleton

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
		virtual bool Exit();

		virtual void ConfigureUserCallback(CallbackPtr userCallback);

		virtual bool Open(long nInputChannels_, long nOutputChannels_, long bufferSize_, unsigned sampleRate_, void *userData_, bool verbose);

	public:

		bool QueryDriverInfo(bool verbose = true);
		bool QueryDeviceInfo(bool verbose = true);

		bool DisposeBuffers();

		// ///
		// Our low level call backs
		static void bufferSwitch(long index, long processNow);
		static void sampleRateChanged(double sRate);
		static long asioMessage(long selector, long value, void* message, double* opt);

		// ///

		bool callbackEvent(long bufferIndex);

		unsigned long SysRefTime();
		long SysMilliSeconds();
		long SamplePos();

		void GetTimeCode(long& hours, long& minutes, long& seconds, double& remainder);

	};

} // end of namespace