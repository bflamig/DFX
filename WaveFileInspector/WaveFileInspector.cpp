#include <iostream>
#include <filesystem>
#include "SoundFile.h"

namespace fs = std::filesystem;
using namespace dfx;

constexpr unsigned raw_nChannels = 1;
constexpr auto raw_format = SampleFormat::SINT16;
constexpr double raw_file_rate = 22050.0;


std::pair<double, double> ComputeStats(SoundFile &f, FrameBuffer<double>& buffer, double duration)
{
	auto nframes = size_t(duration * f.fileRate + 0.5);

	if (nframes > buffer.nFrames)
	{
		nframes = buffer.nFrames;
	}

	const auto nframes_chunk = nframes / 100;

	// Compute the peak signal seen, either channel

	double peak = 0.0;

	for (size_t i = 0; i < nframes; i++)
	{
		// We're just incorporating all channels in the rms
		// we'll fix that afterwards

		if (f.nChannels == 1)
		{
			auto d = std::abs(buffer.GetMonoFrame(i));
			if (d > peak)
			{
				peak = d;
			}
		}
		else if (f.nChannels == 2)
		{
			auto d = buffer.GetStereoFrame(i);
			d.left = std::abs(d.left);
			d.right = std::abs(d.right);

			if (d.left > peak)
			{
				peak = d.left;
			}
			if (d.right > peak)
			{
				peak = d.right;
			}
		}
		else throw std::exception("Too many channels");
	}

	const double threshold = peak / 100.0;

	// Accumulate sum squares statistics

	int accum_n = 0;

	double sum_squared = 0.0;

	size_t i = 0;
	while (i < nframes)
	{
		double chunk_sum_squared = 0.0;

		size_t k;

		for (k = 0; k < nframes_chunk; k++)
		{
			// We're just incorporating all channels in the rms
			// we'll fix that afterwards

			if (f.nChannels == 1)
			{
				auto d = buffer.GetMonoFrame(k);
				chunk_sum_squared += d * d;
			}
			else if (f.nChannels == 2)
			{
				auto d = buffer.GetStereoFrame(k);
				chunk_sum_squared += d.left * d.left;
				chunk_sum_squared += d.right * d.right;
			}
			else throw std::exception("Too many channels");

			if (i++ >= nframes) break;
		}

		auto s = chunk_sum_squared /= f.nChannels;
		s /= k;
		s = sqrt(s);

		if (s >= threshold)
		{
			sum_squared += chunk_sum_squared;

			accum_n += k;
		}
	}

	double rms = sum_squared;

	rms /= f.nChannels;
	if (accum_n > 0) rms /= accum_n; else rms = 0.0;
	rms = sqrt(rms);

	return { peak, rms };
}

int EffectiveBits(int x)
{
	int nd = 0;

	while (true)
	{
		if (x == 0) return nd;
		x /= 2;
		nd++;
	}
}


void ScanFile(const std::string_view& fname, bool raw)
{
	auto f = SoundFile();

	bool b;

	if (raw)
	{
		b = f.OpenRaw(fname, raw_nChannels, raw_format, raw_file_rate);
	}
	else
	{
		b = f.Open(fname);
	}

	if (!b)
	{
		std::cout << "*** Error opening file: " << fname << std::endl;
	}

	std::cout << "Stats for file " << fname << ":\n";
	std::cout << "   Sampling rate:   " << f.fileRate << " Hz" << std::endl;
	std::cout << "   Format:          " << to_string(f.dataType) << std::endl;
	std::cout << "   Channels:        " << f.nChannels << std::endl;
	std::cout << "   Length:          " << f.fileFrames << " frames\n";
	std::cout << "   Duration:        " << f.fileFrames / f.fileRate << " secs\n";

	FrameBuffer<double> buffer(f.fileFrames, f.nChannels);

	bool doNormalize = false;
	f.Read(buffer, 0, doNormalize);

	double window = 0.100; // in seconds

	auto zebra = ComputeStats(f, buffer, window);

	auto peak = std::get<0>(zebra);
	auto rms = std::get<1>(zebra);

	std::cout << "   Raw peak         " << peak << "\n";
	//std::cout << "   Raw RMS          " << rms << " over " << window * 1000.0 << " msecs\n";

	auto x = int(peak + 0.5);
	//std::cout << "   Hex peak         " << std::hex << x << std::dec << "\n";
	std::cout << "   Effective bits   " << EffectiveBits(x) << "\n";

	auto mv = maxVal(f.dataType);

	auto linear_scaled_peak = peak / std::abs(mv.first);  // |first| yields biggest val possible
	auto linear_scaled_rms = rms / std::abs(mv.first);    // ""

	std::cout << "   Normalized peak  " << linear_scaled_peak << "\n";
	std::cout << "   Normalized RMS   " << linear_scaled_rms << " over " << window * 1000.0 << " msecs\n";

	auto db_peak = 20.0 * log10(linear_scaled_peak); // 1.0 basis
	auto db_rms = 20.0 * log10(linear_scaled_rms); // 1.0 basis

	std::cout << "   Relative peak    " << db_peak << " dB\n";
	std::cout << "   Relative rms     " << db_rms << " dB, over " << window * 1000.0 << " msecs\n";

	f.Close();
}


void ScanDir(const fs::path& dir, const fs::path& rel_path, bool recurse, bool raw)
{
	fs::path full_path = dir / rel_path; // NOTE: If rel_path is a full path on its own, it effectively cancels out the dir part.

	fs::directory_entry de(full_path);

	if (de.is_directory())
	{
		fs::directory_iterator di(full_path);

		for (auto d : di)
		{
			if (d.is_directory())
			{
				if (recurse)
				{
					ScanDir(full_path, d.path(), recurse, raw);
				}
			}
			else
			{
				auto ocean = d.path().string();
				ScanFile(ocean, raw);
			}
			std::cout << std::endl;
		}
	}
	else
	{
		auto ocean = full_path.string();
		ScanFile(ocean, raw);
	}
}

void usage(const char* pname)
{
	std::cout << "Usage:" << "\n\n";
	std::cout << "   " << pname << " [-r] [-h] file(or dir) name\n";
	std::cout << "   -r recurse thru sub directories\n";
	std::cout << "   --raw Treat wave file as stk raw (no headers, float32)\n";
	std::cout << "   -h this help\n\n";
}


int main(int argc, const char* argv[])
{
	bool do_raw = false;
	bool recurse = false;

	std::string fname;

	if (argc > 1)
	{
		for (int i = 1; i < argc; i++)
		{
			if (strcmp(argv[i], "-r") == 0)
			{
				recurse = true;
			}
			else if (strcmp(argv[i], "--raw") == 0)
			{
				do_raw = true;
			}
			else if (strcmp(argv[i], "-h") == 0)
			{
				usage(argv[0]);
			}
			else
			{
				fname = argv[i];
			}
		}
	}
	else
	{
		usage(argv[0]);
	}

	if (!fname.empty())
	{
		auto curr_path = fs::current_path();
		auto rel_path = fs::path(fname);
		std::cout << curr_path << std::endl;
		//std::string fname = "S:/Program Files (x86)/NDK/samples/snares/sn10jungle/stx/snare_on/ord/sn10wjungle_stxl_ord_049.wav";
		//std::string fname = "S:/Program Files (x86)/NDK/samples/snares/sn10jungle/stx/snare_on/ord/";
		ScanDir(curr_path, rel_path, recurse, do_raw);
	}
}
