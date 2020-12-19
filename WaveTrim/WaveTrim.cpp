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
		std::vector<double> peaks; // scaled -1 to +1
		std::vector<double> rmss;  // scaled to -1 to +1
		FrameBuffer<int24_t> fb;

		double period;
		double sampleRate;
		unsigned slop;

	public:

		// It's useful to have the start threshold to be bigger than the ending one.
		// We don't get "stuck" as easily when hopping to the next wave and searching
		// for start.

		double start_wave_thold = 0.005;   // Roughly -46dB // Roughly -66dB, where 1.0 is max sample value
		double end_wave_thold   = 0.0001;  // Roughly -80dB, where 1.0 is max sample value

	public:

		VelocityLayerSplitter()
		: ssf{}
		, bounds_map(127)
		, peaks(127)
		, rmss(127)
		, fb{}
		, period{ 4 }
		, sampleRate{ 48000 }
		, slop { unsigned(sampleRate * 20e-3) } // 20 ms worth of slop
		{
		}

		void SetPeriod(double period_)
		{
			period = period_;
		}

		void SetSampleRate(double sampleRate_)
		{
			sampleRate = sampleRate_;
			slop = unsigned(sampleRate * 20e-3); // 20 ms worth of slop
		}

		// Right now, only 24 bit file support

		unsigned FindWaveStart(unsigned beg_start)
		{
			unsigned f = beg_start;
			for (; f < fb.nFrames; f++)
			{
				auto x = fb.GetStereoFrame(f);
				//auto avg = (std::abs(x.left.asDouble()) + std::abs(x.right.asDouble())) / 2.0;
				auto maxxie = fb.GetAbsMaxOfFrame(f).asDouble();
				// Need to scale to our 24 bit world. The 24 bits are in the high three bytes
				// of a 32 bit integer, and then we convert straight to double from there, so ...
				const double scaled_thold = start_wave_thold * 2147483647.0; 
				//if (avg > scaled_thold) break;
				if (maxxie > scaled_thold) break;
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

			unsigned f = start + unsigned(sampleRate * period) - slop; // period secs worth minus some slop

			for (; f > 0; --f)
			{
				auto x = fb.GetStereoFrame(f);
				//auto avg = (std::abs(x.left.asDouble()) + std::abs(x.right.asDouble())) / 2.0;
				auto maxxie = fb.GetAbsMaxOfFrame(f).asDouble();
				// Need to scale to our 24 bit world. The 24 bits are in the high three bytes
				// of a 32 bit integer, and then we convert straight to double from there, so ...
				const double scaled_thold = end_wave_thold * 2147483647.0;
				//if (avg > scaled_thold) break; // because 24 bit in high three bytes
				if (maxxie > scaled_thold) break; // because 24 bit in high three bytes
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
					beg_start = bounds.first + unsigned(sampleRate * period) - slop; // period secs worth minus some slop
				}
			}
			else
			{
				std::cout << "Problemo" << std::endl;
			}
		}

		void ScanForPeaks()
		{
			for (int i = 0; i < 127; i++)
			{
				int24_t peak = fb.FindMax(bounds_map[i].first, bounds_map[i].second + 1);
				peaks[i] = peak.asDouble() / 2147483487.0;
			}
		}

		double ComputePeakRms(unsigned startFrame, unsigned endFrame)
		{
			// endFrame should be one past last frame of interest
			// Right now, 24 bit only

			unsigned extent = endFrame - startFrame; // @@ TODO: bounds check!

			unsigned wd = unsigned(sampleRate * 20e-3); // roughly a 20 ms window

			double peakRms = 0.0;

			for (unsigned j = 0; j < extent; j++)
			{
				double S = 0.0;

				for (unsigned i = 0; i < wd; i++)
				{
					unsigned k = i + wd * j;

					if (k < extent)
					{
						double W = fb.GetAbsMaxOfFrame(k + startFrame).asDouble() / 2147483647.0;
						W = W * W / wd;
						S += W;
					}
					else break;
				}

				double Rms = sqrt(S);
				if (Rms > peakRms) peakRms = Rms;
			}

			return peakRms;
		}

		void ComputeRmss()
		{
			for (int i = 0; i < 127; i++)
			{
				double peakRms = ComputePeakRms(bounds_map[i].first, bounds_map[i].second + 1);
				rmss[i] = peakRms;
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
					dfxi << robinBase << num_str << ".wav" << "\"";
					dfxi << ", peak = " << peaks[i];
					dfxi << ", rms = " << rmss[i];
					dfxi << " }" << std::endl;
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
#if 1
	VelocityLayerSplitter splitter;

	std::filesystem::path basePath = "W:/Reaper";

	std::string drum_name = "Tabla64";
	std::string period_str = "3secs";
	splitter.SetPeriod(3.0);

	std::filesystem::path fname = basePath;
	fname /= drum_name + "." + period_str + ".wav";

	std::string robinPartial = drum_name + "_v";

	std::filesystem::path robinBasePath = basePath;
	robinBasePath /= drum_name + "Robins";
	robinBasePath /= robinPartial;

	std::filesystem::path dfxiPath = basePath;
	dfxiPath /= drum_name;
	dfxiPath.replace_extension("dfxi");

	std::cout << "Finding wave boundaries" << std::endl;
	splitter.FindWaves(fname.generic_string());
	std::cout << "Creating velocity files" << std::endl;
	//splitter.CreateVelocityFiles(robinBasePath.generic_string());
	std::cout << "Finding waveform peaks" << std::endl;
	splitter.ScanForPeaks();
	splitter.ComputeRmss();
	std::cout << "Creating dfxi file" << std::endl;
	splitter.BuildDfxi(dfxiPath.generic_string(), robinPartial);
#else

	SimpleSoundFile ssf;

	//bool b = ssf.OpenForReading("w:/reaper/02-201217_1814.wav");
	bool b = ssf.OpenForReading("w:/reaper/Tabla65.1_5secs.wav");

	switch (ssf.dataType)
	{
		case SampleFormat::SINT16:
		{
			FrameBuffer<int16_t> buff;
			b = ssf.Read(buff);
			auto m = buff.FindMax(4.0);


		}
		break;
		case SampleFormat::SINT24:
		{
			FrameBuffer<int24_t> buff;
			b = ssf.Read(buff);
			auto m = buff.FindMax(4.0);
		}
		break;
		case SampleFormat::SINT32:
		{
			FrameBuffer<int32_t> buff;
			b = ssf.Read(buff);
			auto m = buff.FindMax(4.0);
		}
		break;
		case SampleFormat::FLOAT32:
		{
			FrameBuffer<float> buff;
			b = ssf.Read(buff);
			auto m = buff.FindMax(4.0);
		}
		break;
		case SampleFormat::FLOAT64:
		{
			FrameBuffer<double> buff;
			b = ssf.Read(buff);
			auto m = buff.FindMax(4.0);
		}
		break;
	}

#endif
}

