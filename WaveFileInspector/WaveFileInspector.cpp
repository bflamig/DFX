#include <iostream>
#include <filesystem>
#include "SoundFile.h"

namespace fs = std::filesystem;
using namespace dfx;

constexpr unsigned raw_nChannels = 1;
constexpr auto raw_format = SampleFormat::SINT16;
constexpr double raw_file_rate = 22050.0;

std::tuple<size_t, size_t, double, double, double, double> ComputeStats(SoundFile &f, FrameBuffer<double>& buffer, double duration)
{
#if 0
	auto nframes = size_t(duration * f.fileRate + 0.5);

	if (nframes > buffer.nFrames)
	{
		nframes = buffer.nFrames;
	}
#else
    // Doing auto
	auto nframes = buffer.nFrames;
#endif

	if (nframes == 0)
	{
		throw std::exception("Buffer empty");
	}

	// Scan from the start till we see a signal above a threshold;

	constexpr double start_threshold = 0.0001;

	size_t start;

	for (start = 0; start < nframes; ++start)
	{
		auto x = std::abs(buffer.GetAbsMaxOfFrame(start));

		if (x >= start_threshold) break;
	}

	if (start == nframes)
	{
		// Nothing above threshold, so we'll have to punt
		start = 0; 
	}

	// Scan from the end till we see a signal above a threshold

	constexpr double end_threshold = 0.0001;

	size_t end = nframes;

	while (1)
	{
		auto x = std::abs(buffer.GetAbsMaxOfFrame(--end));
		if (x >= end_threshold) break;
		if (end == 0)
		{
			// Nothing above threshold, so we'll have to punt
			end = nframes;
			break;
		}
	}

	const auto nframes_chunk = nframes / 100;

	// Compute the peak signal seen, either channel

	double neg_peak = 0.0;
	double pos_peak = 0.0;

	for (size_t i = start; i < end; i++)
	{
		auto x = buffer.GetMinOfFrame(i);
		if (x < neg_peak) neg_peak = x;


		auto y = buffer.GetMaxOfFrame(i);
		if (y > pos_peak) pos_peak = y;
	}

	double peak = std::abs(neg_peak) > pos_peak ? std::abs(neg_peak) : pos_peak;

	const double threshold = peak / 100.0;

	// Accumulate sum squares statistics

	int accum_n = 0;

	double sum_squared = 0.0;
	double fallback_sum_squared = 0.0;

	size_t i = start;
	while (i < end)
	{
		double chunk_sum_squared = 0.0;

		size_t k;

		for (k = 0; k < nframes_chunk; k++)
		{
			// We're just incorporating all channels in the rms
			// we'll fix that afterwards

			auto si = k * buffer.nChannels;

			for (size_t j = 0; j < buffer.nChannels; j++)
			{
				auto d = std::abs(buffer.samples[si + j]);
				chunk_sum_squared += d;
			}

			if (i++ >= end) break;
		}

		auto s = chunk_sum_squared / buffer.nChannels;
		s /= k;
		s = sqrt(s);

		if (s >= threshold)
		{
			sum_squared += chunk_sum_squared;
			accum_n += k;
		}

		fallback_sum_squared += chunk_sum_squared;
	}

	double rms;
	 
	if (accum_n > 0)
	{
		rms = sum_squared / buffer.nChannels;
		rms /= accum_n;
	}
	else
	{
		rms = fallback_sum_squared / buffer.nChannels;
		rms /= nframes;
	}

	rms = sqrt(rms);

	return { start, end, neg_peak, pos_peak, peak, rms };
}

template<typename T>
int EffectiveIntegerBits(T x)
{
	int nd = 0;

	while (true)
	{
		if (x == 0) return nd;
		x /= 2;
		nd++;
	}

	return nd;
}

int EffectiveBits(double x, SampleFormat file_type)
{
	switch (file_type)
	{
		case SampleFormat::SINT16:
		{
			auto d = int16_t(x * 32767.5 - 0.5);
			return EffectiveIntegerBits(d);

		}
		break;
		case SampleFormat::SINT24:
		{
			auto d = int32_t(x * 8388607.5 - 0.5);
			return EffectiveIntegerBits(d);

		}
		break;
		case SampleFormat::SINT32:
		{
			auto d = int32_t(x * 2147483647.5 - 0.5);
			return EffectiveIntegerBits(d);
		}
		break;
		case SampleFormat::FLOAT32:
		case SampleFormat::FLOAT64:
		{
			x += 0.5;
			x /= 2147483647.5;
			x = int32_t(x);
			return EffectiveIntegerBits(x);
		}
	}

	throw std::exception("Invalid sample format");
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

	bool doNormalize = true;
	f.Read(buffer, 0, doNormalize);

	double window = 0.100; // in seconds

	auto zebra = ComputeStats(f, buffer, window);

	auto start = std::get<0>(zebra);
	auto end = std::get<1>(zebra);
	auto neg_peak = std::get<2>(zebra);
	auto pos_peak = std::get<3>(zebra);
	auto peak = std::get<4>(zebra);
	auto rms = std::get<5>(zebra);

	std::cout << "   Start            " << start << " (" << (start / f.fileRate) << ") secs" << std::endl;
	std::cout << "   End              " << end << " (" << (end / f.fileRate) << ") secs" << std::endl;
	std::cout << "   Neg peak         " << neg_peak << std::endl;
	std::cout << "   Pos peak         " << pos_peak << std::endl;

	auto x = int(peak + 0.5);
	std::cout << "   Effective bits   " << EffectiveBits(peak, f.dataType) << std::endl;

	auto mv = maxVal(f.dataType);

	auto linear_scaled_peak = peak;
	auto linear_scaled_rms = rms;

	std::cout << "   Normalized peak  " << linear_scaled_peak << std::endl;
	std::cout << "   Normalized RMS   " << linear_scaled_rms << std::endl;

	auto db_peak = 20.0 * log10(linear_scaled_peak); // 1.0 basis
	auto db_rms = 20.0 * log10(linear_scaled_rms); // 1.0 basis

	std::cout << "   Relative peak    " << db_peak << " dB" << std::endl;
	std::cout << "   Relative rms     " << db_rms << " dB" << std::endl;

	std::cout << "   Est Midi vel     " << int(rms * 127.0) << std::endl;

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
