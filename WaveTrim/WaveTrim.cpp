#include <iostream>
#include <ios>
#include <fstream>
#include <filesystem>
#include "SimpleSoundFile.h"


namespace dfx
{

	// Right now, only 24 bit file support

	class VelocityLayerSplitter {
	public:

		SimpleSoundFile ssf;
		std::vector<std::pair<unsigned, unsigned>> bounds_map;
		FrameBuffer<int24_t> fb;

	public:

		// It's useful to have the start threshold to be bigger than the ending one.
		// We don't get "stuck" as easily when hopping to the next wave.

		double start_wave_thold = 0.0005;  // Roughly -66dB, where 1.0 is max sample value
		double end_wave_thold   = 0.0001;  // Roughly -80dB, where 1.0 is max sample value

	public:

		VelocityLayerSplitter()
		: ssf{}
		, bounds_map(127)
		, fb{}
		{

		}

		// Right now, only 24 bit file support

		unsigned FindWaveStart(unsigned beg_start)
		{
			unsigned f = beg_start;
			for (; f < fb.nFrames; f++)
			{
				auto x = fb.GetStereoFrame(f);
				auto avg = (std::abs(x.left.asDouble()) + std::abs(x.right.asDouble())) / 2.0;
				// Need to scale to our 24 bit world. The 24 bits are in the high three bytes
				// of a 32 bit integer, and then we convert straight to double from there, so ...
				const double scaled_thold = start_wave_thold * 2147483647.0; 
				if (avg > scaled_thold) break;
			}
			return f;
		}

		unsigned FindWaveEnd(unsigned start)
		{
			// @@ Value returned is *right* at the end, not one past it.

			// We skip to near the start of the next wave, and then scan backwards
			// We need some "slop" so that we don't land on top of the next wave
			// and cause all sorts of problems. The reason we need slop? It's because
			// we can't guarantee the waves are the same number of samples apart
			// each time. They would be, but there are vagaries in sending midi commands
			// (how we generate the waves) and DAW processing, etc. There is a bit of
			// jitter in the timing. The below seems to work at least for 4 sec intervals
			// at 48 kHz sampling.

			unsigned f = start + 48000 * 4 - 1000; // 4 secs worth minus some slop

			for (; f > 0; --f)
			{
				auto x = fb.GetStereoFrame(f);
				auto avg = (std::abs(x.left.asDouble()) + std::abs(x.right.asDouble())) / 2.0;
				// Need to scale to our 24 bit world. The 24 bits are in the high three bytes
				// of a 32 bit integer, and then we convert straight to double from there, so ...
				const double scaled_thold = end_wave_thold * 2147483647.0;
				if (avg > scaled_thold) break; // because 24 bit in high three bytes
			}

			return f;
		}

		std::pair<unsigned, unsigned> FindWave(unsigned beg_start)
		{
			// Finds the bounds of one wave, given an initial guess for the start.
			auto start = FindWaveStart(beg_start);
			auto end = FindWaveEnd(start);
			return { start, end };
		}

		void FindWaves(const std::string &fname)
		{
			//std::string fname = "W:\\TestSample.wav";
			//std::string fname = "W:\\Reaper\\Tom4.wav";

			//SimpleSoundFile ssf;

			std::cout << "Reading wave file: " << fname << std::endl;

			bool b = ssf.OpenForReading(fname);

			if (b)
			{
				b = ssf.Read(fb);
				ssf.Close();

				unsigned beg_start = 0;

				for (int i = 0; i < 127; i++)
				{
					auto bounds = FindWave(beg_start);
					std::cout << "i: " << (i + 1) << " start = " << bounds.first << " end = " << bounds.second << std::endl;
					bounds_map[i] = bounds;
					beg_start = bounds.first + 48000 * 4 - 1000;
				}
			}
			else
			{
				std::cout << "Problemo" << std::endl;
			}
		}

		void CreateVelocityFiles(const std::string &robinBasePath)
		{
			// std::string robinBasePath = "W:/Reaper/Robins/Tom4_v";

			for (int i = 0; i < 127; i++)
			{
				std::string robinPath = robinBasePath + std::to_string(i + 1) + ".wav";

				bool b = ssf.OpenForWriting(robinPath, fb);

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
		}

		bool BuildDfxi(const std::string &soundFontPath, const std::string& robinBase)
		{
			// Nice utility to create a dfxi file, (dfx include file)

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

	};

} // End of namespace

using namespace dfx;

int main()
{
	VelocityLayerSplitter splitter;

	std::string drum_name = "Tom2";

	std::filesystem::path basePath = "W:/Reaper";

	std::filesystem::path fname = basePath;
	fname /= drum_name;
	fname.replace_extension("wav");

	//std::string fname = "W:/TestSample.wav";
	// std::string fname = "W:/Reaper/Tom4.wav";

	std::string robinPartial = drum_name + "_v";

	std::filesystem::path robinBasePath = basePath;
	robinBasePath /= drum_name + "Robins";
	robinBasePath /= robinPartial;

	// std::string robinBasePath = "W:/Reaper/Robins/Tom4_v";

	std::filesystem::path dfxiPath = basePath;
	dfxiPath /= drum_name;
	dfxiPath.replace_extension("dfxi");

	splitter.FindWaves(fname.generic_string());
	splitter.CreateVelocityFiles(robinBasePath.generic_string());
	splitter.BuildDfxi(dfxiPath.generic_string(), robinPartial);
}

