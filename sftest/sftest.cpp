// sftest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <filesystem>
#include "SoundFile.h"

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

		bool raw = true;
		bool rv = sf.OpenStkRaw(waveFile1, 1, SampleFormat::SINT16, 22050.0);

		raw = false;
		rv = sf.Open(waveFile2);
	}

} // end of namespace

int main()
{
	dfx::test1();
}

