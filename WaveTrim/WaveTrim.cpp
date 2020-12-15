// WaveTrim.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <ios>
#include <fstream>
#include <filesystem>
#include "SimpleSoundFile.h"

using namespace dfx;

//template<class T>
unsigned FindWaveStart(FrameBuffer<int24_t>& fb, unsigned beg_start)
{
	unsigned f = beg_start;
	for (; f < fb.nFrames; f++)
	{
		auto x = fb.GetStereoFrame(f);
		auto avg = (std::abs(x.left.asDouble()) + std::abs(x.right.asDouble())) / 2.0;
		const double thold = 0.0005 * 2147483647.0;
		if (avg > thold) break;
	}


	return f;
}

unsigned FindWaveEnd(FrameBuffer<int24_t>& fb, unsigned start)
{
	// @@ Value returned is *right* at the end, not one past it.

	unsigned f = start + 48000 * 4 - 1000; // 4 secs worth minus some slop

	for (; f > 0; --f)
	{
		auto x = fb.GetStereoFrame(f);
		auto avg = (std::abs(x.left.asDouble()) + std::abs(x.right.asDouble())) / 2.0;
		const double thold = 0.0001 * 2147483647.0;
		if (avg > thold) break; // because 24 bit in high three bytes
	}

	return f;
}

std::pair<unsigned, unsigned> FindWave(FrameBuffer<int24_t>& fb, unsigned beg_start)
{
	auto start = FindWaveStart(fb, beg_start);
	auto end = FindWaveEnd(fb, start);
	return { start, end };
}


bool BuildDfxi(const std::string& soundFontPath, const std::string &robinBase)
{
	std::fstream dfxi(soundFontPath, std::ios::out);

	if (dfxi.is_open())
	{
		dfxi << "dfxi =" << std::endl;
		dfxi << "{" << std::endl;

		dfxi << "    velocities = " << std::endl;
		dfxi << "    [" << std::endl;
		for (int i = 0; i < 127; i++)
		{
			auto num_str = std::to_string(i + 1);
			dfxi << "        " << "vr" << num_str << " = { fname = \"";
			dfxi << robinBase << num_str << ".wav" << "\"" << " }" << std::endl;
		}

		dfxi << "    ]" << std::endl;
		dfxi << "}" << std::endl;

		return true;
	}
	else
	{
		return false;
	}
}

void Doofer()
{
	//std::string fname = "W:\\TestSample.wav";
	std::string fname = "W:\\Reaper\\Tom4.wav";

	SimpleSoundFile ssf;

	bool b = ssf.OpenForReading(fname);

	if (b)
	{
		FrameBuffer<int24_t> fb;
		b = ssf.Read(fb);
		ssf.Close();


		std::vector<std::pair<unsigned, unsigned>> bounds_map(127);

		unsigned beg_start = 0;


		for (int i = 0; i < 127; i++)
		{
			auto bounds = FindWave(fb, beg_start);
			std::cout << "i: " << (i + 1) << " start = " << bounds.first << " end = " << bounds.second << std::endl;
			bounds_map[i] = bounds;
			beg_start = bounds.first + 48000 * 4 - 1000;
		}

#if 0

		std::string robinBasePath = "W:/Reaper/Robins/Tom4_v";

		for (int i = 0; i < 127; i++)
		{
			std::string robinPath = robinBasePath + std::to_string(i + 1) + ".wav";

			b = ssf.OpenForWriting(robinPath, fb);

			if (b)
			{
				// NOTE: In the writing here, we set the end to be *one past the end*.
				// This is to keep the semantics the same as if we were to use an iterator.
				// It keeps me sane!
				unsigned hacked_first = bounds_map[i].first;
				if (hacked_first > 16) hacked_first -= 16;
				ssf.Write(fb, bounds_map[i].first, bounds_map[i].second + 1);
				ssf.Close();
			}
		}

#endif

		std::filesystem::path soundFontPath = fname;
		soundFontPath.replace_filename("Tom4.dfxi");

		std::fstream dfxi(soundFontPath, std::ios::out);

		std::string robinBasePath = "W:/Reaper/Robins/Tom4_v";

		if (dfxi.is_open())
		{
			dfxi << "dfxi = {" << std::endl;
			dfxi << "    velocities = " << std::endl;
			dfxi << "    [" << std::endl;

			for (int i = 0; i < 127; i++)
			{
				dfxi << "        " << "vr" << std::to_string(i + 1) << " = { fname = \"";
				dfxi << robinBasePath << std::to_string(i + 1) << ".wav" << "\"" << " }" << std::endl;
			}

			dfxi << "    ]" << std::endl;
			dfxi << "}" << std::endl;
		}
		else
		{
			std::cout << "Having trouble opening dfxi file" << std::endl;
		}


	}
	else
	{
		std::cout << "Problemo" << std::endl;
	}

}

int main()
{
	BuildDfxi("W:/Reaper/Tom4.dfxi", "Tom4_v");
}

