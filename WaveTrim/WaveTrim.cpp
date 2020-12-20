#include <iostream>
#include <ios>
#include <fstream>
#include <filesystem>
#include <cmath>
#include <algorithm>
#include "WaveFile.h"


namespace dfx
{
	// Right now, only 24 bit wave file support

	class VelocityLayerSplitter {
	public:

		WaveFile wf;
		std::vector<std::pair<unsigned, unsigned>> bounds_map;
		std::vector<int> vel_map;  // max velocity of each range
		std::vector<double> peaks; // scaled -1 to +1
		std::vector<double> rmss;  // scaled to -1 to +1
		FrameBuffer<int24_t> fb;   // 24-bit only at the moment

		double period;
		double sampleRate;
		unsigned slop;

		static constexpr double sloppy_seconds = 20e-3; // OMG

		int num_vels;

	public:

		// It's useful to have the start threshold to be bigger than the ending one.
		// We don't get "stuck" as easily when hopping to the next wave and searching
		// for start.

		double start_wave_thold = 0.005;   // Roughly -46dB // Roughly -66dB, where 1.0 is max sample value
		double end_wave_thold   = 0.0001;  // Roughly -80dB, where 1.0 is max sample value

	public:

		VelocityLayerSplitter(int num_vels_, double period_)
		: wf{}
		, bounds_map{}
		, vel_map{}
		, peaks{}
		, rmss{}
		, fb{}
		, period{ period_ }
		, sampleRate{ 48000 }
		, slop { unsigned(sampleRate * sloppy_seconds) } // So many samples worth of slop
		, num_vels { num_vels_ }
		{
			bounds_map.reserve(num_vels);
			vel_map.reserve(num_vels);
			peaks.reserve(num_vels);
			rmss.reserve(num_vels);
			CreateVelMap();
		}

		void SetPeriod(double period_)
		{
			period = period_;
		}

		void SetSampleRate(double sampleRate_)
		{
			sampleRate = sampleRate_;
			slop = unsigned(sampleRate * sloppy_seconds); // So many samples worth of slop
		}

		void CreateVelMap()
		{
			double spread = 127.0 / num_vels;

			for (int i = 1; i <= num_vels; i++)
			{
				int m = static_cast<int>(spread * i + 0.5);
				vel_map.push_back(m);
			}
		}

		unsigned FindWaveStart(unsigned beg_start)
		{
			// REMINDER: Right now, only 24 bit file support

			unsigned f = beg_start;
			for (; f < fb.nFrames; f++)
			{
				auto x = fb.GetStereoFrame(f);
				//auto avg = (std::abs(x.left.asDouble()) + std::abs(x.right.asDouble())) / 2.0;
				auto maxxie = fb.GetAbsMaxOfFrame(f).asDouble();
				// Need to scale to our 24 bit world. The 24 bits are moved into the high three bytes
				// of a 32 bit integer, and then we convert straight to double from there, so ...
				const double scaled_thold = start_wave_thold * 2147483647.0; 
				//if (avg > scaled_thold) break;
				if (maxxie > scaled_thold) break;
			}
			return f;
		}

		unsigned FindWaveEnd(unsigned start)
		{
			// REMINDER: Right now, only 24 bit file support

			// @@ Value returned is *right* at the end, not one past it.

			// The "start" argument coming in is the start of the wave we are
			// currently on. 

			// We skip to the vicinity right before the start of the *next* wave, and then
			// scan backwards. We need some "slop" so that we don't land on top of the
			// next wave and cause all sorts of problems. The reason we need slop? It's
			// because we can't guarantee the waves are the same number of samples apart
			// each time. The vagaries of midi timing and DAW audio processing make this
			// impossible (so it seems anyway). Anticipate a bit of jitter in the timing.
			// The code below seems to work -- at least for 4 sec intervals at 48 kHz sampling.

			unsigned skip_delta = unsigned(sampleRate * period) - slop; // number of samples to skip to
			unsigned f = start + skip_delta;

			// Make sure not to start past the end of the buffer!

			if (f >= fb.nFrames)
			{
				// @@ TODO: Really, should we just get out? It means we didn't run the wave sampling
				// long enough during recording. Hmmmm... 
				f = fb.nFrames;
			}

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

			//WavFile ssf;

			std::cout << "Reading wave file: " << fname << std::endl;

			bool b = wf.OpenForReading(fname);

			if (b)
			{
				b = wf.Read(fb);
				wf.Close();

				unsigned beg_start = 0;

				for (int i = 0; i < num_vels; i++)
				{
					auto bounds = FindWave(beg_start);
					std::cout << "v: " << (vel_map[i]) << " start = " << bounds.first << " end = " << bounds.second << std::endl;
					//bounds_map[i] = bounds;
					bounds_map.push_back(bounds);
					unsigned skip_delta = unsigned(sampleRate * period) - slop; // this many samples to skip
					beg_start = bounds.first + skip_delta; // period secs worth minus some slop
				}
			}
			else
			{
				std::cout << "Problemo" << std::endl;
			}
		}

		void ScanForPeaks()
		{
			for (int i = 0; i < num_vels; i++)
			{
				int24_t peak = fb.FindMax(bounds_map[i].first, bounds_map[i].second + 1);
				auto p = peak.asDouble() / 2147483487.0;
				peaks.push_back(p);
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
			for (int i = 0; i < num_vels; i++)
			{
				double peakRms = ComputePeakRms(bounds_map[i].first, bounds_map[i].second + 1);
				rmss.push_back(peakRms);
			}
		}


		void SortByPeaks()
		{
			//std::sort(bounds_map.begin(), bounds_map.end(), )
		}

		void CreateVelocityFiles(const std::string &robinBasePath)
		{
			for (int i = 0; i < num_vels; i++)
			{
				std::string robinPath = robinBasePath + std::to_string(vel_map[i]) + ".wav";

				bool b = wf.OpenForWriting(robinPath, fb);

				if (b)
				{
					// NOTE: In the writing here, we set the end to be *one past the end*.
					// This is to keep the semantics the same as if we were to use it in
					// an iterator setting.
					// It keeps me sane!

					unsigned hacked_first = bounds_map[i].first;
					if (hacked_first > 16) hacked_first -= 16;
					wf.Write(fb, bounds_map[i].first, bounds_map[i].second + 1);
					wf.Close();
				}
			}
		}

		bool BuildDfxi(const std::string &dfxiPath, const std::string& robinBase)
		{
			// Nice utility to create a dfxi file, (dfx include file)

			std::fstream dfxi(dfxiPath, std::ios::out);

			if (dfxi.is_open())
			{
				dfxi << "dfxi =" << std::endl;
				dfxi << "{" << std::endl;

				dfxi << "    velocities = " << std::endl;
				dfxi << "    [" << std::endl;
				for (int i = 0; i < num_vels; i++)
				{
					auto num_str = std::to_string(vel_map[i]);
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

void doit(const std::string_view basePath, const std::string_view drum_name, int num_hits, double period)
{
	VelocityLayerSplitter splitter(num_hits, period);

	double w;
	auto frac = std::modf(period, &w);

	auto x = static_cast<int>(w);
	auto f = static_cast<int>(frac * 10 + 0.5);

	std::string frog = (f == 0) ? "" : "_" + std::to_string(f);

	std::string period_str = std::to_string(x) + frog + "secs";

	std::filesystem::path fname = basePath;
	fname /= drum_name;
	fname += "." + period_str + ".wav";

	std::string robinPartial = drum_name.data();
	robinPartial += "_v";

	std::filesystem::path wavesDir = basePath;
	wavesDir /= drum_name;
	wavesDir += "Robins";

	std::filesystem::path robinBasePath = wavesDir;
	robinBasePath /= robinPartial;

	std::filesystem::path dfxiPath = wavesDir;
	dfxiPath /= drum_name;
	dfxiPath.replace_extension("dfxi");

	std::cout << "Finding wave boundaries" << std::endl;
	splitter.FindWaves(fname.generic_string());
	std::cout << "Creating velocity files" << std::endl;
	splitter.CreateVelocityFiles(robinBasePath.generic_string());
	std::cout << "Finding waveform peaks" << std::endl;
	splitter.ScanForPeaks();
	splitter.ComputeRmss();
	std::cout << "Creating dfxi file" << std::endl;
	splitter.BuildDfxi(dfxiPath.generic_string(), robinPartial);
}

int main()
{
#if 1
	//doit("W:/Reaper", "Tabla62", 127, 3.5);
	//doit("W:/Reaper", "Tabla63", 127, 4);
	//doit("W:/Reaper", "Tabla65", 127, 1.5);
	doit("W:/Reaper", "Tabla66", 127, 1);
#else

	int num_hits = 127;

	VelocityLayerSplitter splitter(num_hits, 3.5);

	std::filesystem::path basePath = "W:/Reaper";

	std::string drum_name = "Tabla62";
	std::string period_str = "3_5secs";

	std::filesystem::path fname = basePath;
	fname /= drum_name + "." + period_str + ".wav";

	std::string robinPartial = drum_name + "_v";

	std::filesystem::path wavesDir = basePath;
	wavesDir /= drum_name + "Robins";

	std::filesystem::path robinBasePath = wavesDir;
	robinBasePath /= robinPartial;

	std::filesystem::path dfxiPath = wavesDir;
	dfxiPath /= drum_name;
	dfxiPath.replace_extension("dfxi");

	std::cout << "Finding wave boundaries" << std::endl;
	splitter.FindWaves(fname.generic_string());
	//std::cout << "Creating velocity files" << std::endl;
	//splitter.CreateVelocityFiles(robinBasePath.generic_string());
	std::cout << "Finding waveform peaks" << std::endl;
	splitter.ScanForPeaks();
	splitter.ComputeRmss();
	//std::cout << "Creating dfxi file" << std::endl;
	//splitter.BuildDfxi(dfxiPath.generic_string(), robinPartial);

#endif

#if 0

	WavFile ssf;

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

