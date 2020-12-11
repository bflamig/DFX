// sftest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <filesystem>
#include "MemWave.h"

namespace dfx
{
#ifdef __OS_WINDOWS__
	std::string waveFile1("G:/DrumSW/WaveLibrary/FakeWaves_raw/fakesnare/42/42_dee1.raw");
	std::string waveFile2("G:/DrumSW/WaveLibrary/DownloadedWaves/FocusRite/Snare_Rods_Flam/Snare_Rods_Flam.wav");
#else
	std::string waveFile1("/home/pi/WaveLibrary/FakeWaves_raw/fakesnare/42/42_dee1.raw");
	std::string waveFile2("/home/pi/WaveLibrary/DownloadedWaves/FocusRite/Snare_Rods_Flam/Snare_Rods_Flam.wav");
#endif


	void test1()
	{
		SoundFile sf;

		bool rv = sf.OpenRaw(waveFile1, 1, SampleFormat::SINT16, 22050.0);

		rv = sf.Open(waveFile2);
	}

	void test2()
	{
		MemWave mw;

		bool b = mw.LoadRaw(waveFile1, 1, SampleFormat::SINT16, 22050.0);

		// Test reading only a portion of the file in.

		unsigned start_frame = 100;
		unsigned end_frame = 4904257; // If end is 0, it means "to the end of the file")

		b = mw.Load(waveFile2, start_frame, end_frame);

		if (!b)
		{
			mw.sound_file.LastError().Print(std::cout);
		}
	}

} // end of namespace

int main()
{
	//dfx::test1();
	dfx::test2();
}

