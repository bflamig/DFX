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

#include "AsioMgr.h"
#include "MemWave.h"
#include <iostream>
#include <sstream>
#include <string>
//#include <filesystem>

const char* const ASIO_DRIVER_NAME = "Focusrite USB ASIO";
//const char* const ASIO_DRIVER_NAME = "JRiver Media Center 26";
//const char* const ASIO_DRIVER_NAME = "ReaRoute ASIO (x64)";
//const char* const ASIO_DRIVER_NAME = "UMC ASIO Driver";

using namespace dfx;

void ListDevices()
{
	AsioMgr am;

	auto nd = am.NumDevices();

	auto names = am.DeviceNames();

	std::cout << "-------------------------------------------" << std::endl;
	std::cout << "Devices: " << std::endl;

	for (int i = 0; i < nd; i++)
	{
		std::cout << i << ": " << names[i] << std::endl;
	}

	std::cout << "-------------------------------------------" << std::endl;

	std::cout << std::endl;
}

int loopBack(void* outBuff, void* inBuff, unsigned nFrames, double streamTime, StreamIOStatus ioStatus, void* userData)
{
	// A callback function that just sends input to output. A great way to test the 
	// audio streaming. WARNING: Have this go to your headphones, not your monitors
	// if the inputs are microphones. Otherwise, feedback city!

	// WARNING! Runs in a different thread! (The ASIO thread)

	auto p = reinterpret_cast<system_t*>(outBuff);
	auto q = reinterpret_cast<system_t*>(inBuff);

	for (unsigned i = 0; i < nFrames * 2; i++)
	{
		*p++ = *q++;
	}

	return (streamTime > 2) ? 2 : 0;
}

void test1()
{
	AsioMgr am;

	bool verbose = true;

	// load the driver, this will setup all the necessary internal data structures
	if (am.LoadDriver(ASIO_DRIVER_NAME))
	{
		// initialize the driver
		if (am.InitDriver(verbose))
		{
			//am.PopupControlPanel(); // @@ NOT BLOCKING. SO not much good!

			// set up the asioCallback structure and create the ASIO data buffer

			am.ConfigureUserCallback(loopBack);

			// 2 ins, 2 outs (aka stereo)
			// 64 sample buffers (nominal)

			if (am.Open(2, 2, 64, 48000, nullptr, verbose))
			{
				if (am.Start())
				{
					// Now all is up and running
					std::cout << "\nASIO session started successfully.\n\n";

					while (!am.Stopped())
					{
#if WINDOWS
						Sleep(100);	// goto sleep for 100 milliseconds
#elif MAC
						unsigned long dummy;
						Delay(6, &dummy);
#endif

						fprintf(stdout, "%lf stream time", am.getStreamTime());
						fprintf(stdout, "     \r");
						fflush(stdout);
					}
				}
			}

			am.Close();
		}
	}
}


bool Prep(DfxAudio& da, const std::string& driver_name, bool verbose)
{
	bool b = da.LoadDriver(driver_name);

	if (b)
	{
		b = da.InitDriver(verbose);
	}
	else std::cout << "Trouble loading and initializing driver" << std::endl;

	return b;
}

struct MyData
{
	FrameBuffer<system_t>& fb;
	unsigned samplesPlayed;

	MyData(FrameBuffer<system_t>& fb_) : fb(fb_), samplesPlayed{} {; }
};

int loopPlayBack(void* outBuff, void* inBuff, unsigned nFrames, double streamTime, StreamIOStatus ioStatus, void* userData)
{
	// A callback function that repeatedly plays back a sound file. A great way to test the 
	// playback audio streaming. We don't use the inBuff.

	// WARNING! Runs in a different thread! (The ASIO thread)

	auto data = reinterpret_cast<MyData*>(userData);
	auto& w = data->fb;

	auto p = reinterpret_cast<system_t*>(outBuff);

	auto nOutChannels = data->fb.nChannels; // @@ TODO: THIS BETTER MATCH what we opened!

	unsigned left_over = 0;

	if (data->samplesPlayed >= w.nFrames)
	{
		data->samplesPlayed = 0; // play repeatedly for now for testing purposes
	}

	for (unsigned i = 0; i < nFrames; i++)
	{
		auto sf = w.GetStereoFrame(data->samplesPlayed++);
		*p++ = sf.left;
		*p++ = sf.right;
	}

	for (unsigned i = 0; i < left_over; i++)
	{
		*p++ = 0;
		*p++ = 0;
	}

	return 0;
}

static int64_t cowboy = 0;
static int64_t indian = 0;
static double buffalo = 0.0;

int wavesPlayBack(void* outBuff, void* inBuff, unsigned nFrames, double streamTime, StreamIOStatus ioStatus, void* userData)
{
	// A callback function that repeatedly plays back a sound file. A great way to test the 
	// playback audio streaming. We don't use the inBuff.

	// WARNING! Runs in a different thread! (The ASIO thread)

	// We don't use streamTime here. Instead, the MemWave object keeps track of
	// where we are, and knows when we are finished. It may be doing interpolation
	// of samples on the fly.

	auto waves = reinterpret_cast<MemWave*>(userData);
	auto p = reinterpret_cast<system_t*>(outBuff);

	auto nOutChannels = waves->buff.nChannels; // @@ TODO: THIS BETTER MATCH what we opened!

	if (waves->IsFinished())
	{
		for (unsigned i = 0; i < nFrames; i++)
		{
			*p++ = 0;
			*p++ = 0;
		}

		return 2;
	}

	++indian;
	buffalo = streamTime;

	if (nOutChannels == 1)
	{
		while (nFrames > 0)
		{
			// NOTE: We are ticking at the playback sampling rate, which may not
			// be the sampling rate of the recorded file. So the tick function
			// below might have to calculate an interpolated frame.
			auto sf = waves->MonoTick();
			*p++ = sf;
			*p++ = sf;
			--nFrames;
			++cowboy;
		}
	}
	else
	{
		while (nFrames > 0)
		{
			// NOTE: We are ticking at the playback sampling rate, which may not
			// be the sampling rate of the recorded file. So the tick function
			// below might have to calculate an interpolated frame.
			auto sf = waves->StereoTick();
			*p++ = sf.left;
			*p++ = sf.right;
			--nFrames;
			++cowboy;
		}
	}

	return 0;
}


#ifdef __OS_WINDOWS__
//std::string waveFile1("G:/DrumSW/WaveLibrary/FakeWaves_raw/fakesnare/42/42_dee1.raw");
std::string waveFile1("G:/DrumSW/WaveLibrary/FakeWaves_raw/fakesnare/108/108_cowbell1.raw");
//std::string waveFile1("G:/DrumSW/WaveLibrary/FakeWaves_raw/fakeslap/tambourn.raw");
//std::string waveFile1("G:/DrumSW/WaveLibrary/FakeWaves_raw/fakesnare/100/100_dope.raw");
std::string waveFile2("G:/DrumSW/WaveLibrary/DownloadedWaves/FocusRite/Snare_Rods_Flam/Snare_Rods_Flam.wav");
#else
std::string waveFile1("/home/pi/WaveLibrary/FakeWaves_raw/fakesnare/42/42_dee1.raw");
std::string waveFile1("/home/pi/WaveLibrary/FakeWaves_raw/fakesnare/108/108_cowbell1.raw");
std::string waveFile2("/home/pi/WaveLibrary/DownloadedWaves/FocusRite/Snare_Rods_Flam/Snare_Rods_Flam.wav");
#endif

void testWave(const std::string_view waveFile)
{
	FrameBuffer<double> frames;
	SoundFile sf;

	bool rv = sf.Open(waveFile);
	if (!rv)
	{
		auto& last_err = sf.LastError();
		last_err.Print(std::cout);
		return;
	}

	frames.Resize(sf.fileFrames, sf.nChannels);

	frames.SetDataRate(48000);

	rv = sf.Read(frames, 0, true);
	if (!rv)
	{
		auto& last_err = sf.LastError();
		last_err.Print(std::cout);
		return;
	}

	AsioMgr da;
	bool verbose = true;

	bool b = Prep(da, ASIO_DRIVER_NAME, verbose);

	da.ConfigureUserCallback(loopPlayBack);

	MyData myData(frames);

	// 0 ins, 2 outs (aka stereo)
	// 64 sample buffers (nominal)

	b = da.Open(0, 2, 64, static_cast<unsigned>(frames.dataRate), &myData, verbose);

	if (b)
	{
		b = da.Start();
	}
	else std::cout << "Error opening stream" << std::endl;

	auto start = std::chrono::high_resolution_clock::now();

	if (b)
	{
		std::cout << "\nASIO session started successfully.\n" << std::endl;

		while (!da.Stopped())
		{
			//fprintf(stdout, "%lf stream time\n", am.getStreamTime());
		}

		auto finish = std::chrono::high_resolution_clock::now();

		auto elapsed = finish - start;
		auto zebra = elapsed.count();

		std::cout << "Session ended. Playing time = " << zebra / 1.0e9 << " secs" << std::endl;
	}
	else std::cout << "Error starting stream" << std::endl;

	//da.Close();
}


void testMemWaveII(const std::string_view waveFile)
{
	MemWave waves;

	bool rv = waves.Load(waveFile);
	if (!rv)
	{
		auto& last_err = waves.sound_file.LastError();
		last_err.Print(std::cout);
		return;
	}

	waves.SetRate(48000);

	AsioMgr da;
	bool verbose = true;

	bool b = Prep(da, ASIO_DRIVER_NAME, verbose);

	da.ConfigureUserCallback(wavesPlayBack);

	// 0 ins, 2 outs (aka stereo)
	// 64 sample buffers (nominal)

	b = da.Open(0, 2, 64, static_cast<unsigned>(waves.sampleRate), &waves, verbose);

	auto start = std::chrono::high_resolution_clock::now();

	if (b)
	{
		b = da.Start();
	}
	else std::cout << "Error opening stream" << std::endl;

	if (b)
	{
		std::cout << "\nAudio session started successfully.\n" << std::endl;

		while (!da.Stopped())
		{
			//fprintf(stdout, "%lf stream time\n", am.getStreamTime());
		}

		auto finish = std::chrono::high_resolution_clock::now();

		auto elapsed = finish - start;
		auto zebra = elapsed.count();

		std::cout << "Session ended. Playing time = " << zebra / 1.0e9 << " secs" << std::endl;
	}
	else std::cout << "Error starting stream" << std::endl;

	//da.Close();
}

void testRaw(const std::string_view waveFile)
{
	MemWave waves;

	bool rv = waves.LoadRaw(waveFile1, 1, SampleFormat::SINT16, 22050.0);
	if (!rv)
	{
		auto& last_err = waves.sound_file.LastError();
		last_err.Print(std::cout);
		return;
	}

	waves.SetRate(48000);

	AsioMgr da;
	bool verbose = true;

	bool b = Prep(da, ASIO_DRIVER_NAME, verbose);

	da.ConfigureUserCallback(wavesPlayBack);

	// 0 ins, 2 outs (aka stereo)
	// 64 sample buffers (nominal)

	b = da.Open(0, 2, 64, static_cast<unsigned>(waves.sampleRate), &waves, verbose);

	auto start = std::chrono::high_resolution_clock::now();

	if (b)
	{
		b = da.Start();
	}
	else std::cout << "Error opening stream" << std::endl;

	if (b)
	{
		std::cout << "\nAudio session started successfully.\n" << std::endl;

		while (!da.Stopped())
		{
			//fprintf(stdout, "%lf stream time\n", am.getStreamTime());
		}

		auto finish = std::chrono::high_resolution_clock::now();

		auto elapsed = finish - start;
		auto zebra = elapsed.count();

		std::cout << "Session ended. Playing time = " << zebra / 1.0e9 << " secs" << std::endl;
	}
	else std::cout << "Error starting stream" << std::endl;

	//da.Close();
}


int main(int argc, char* argv[])
{
	ListDevices();
	//test1();
	//testWave(waveFile2);
	testMemWaveII(waveFile2);
	//testRaw(waveFile1);
	return 0;
}
