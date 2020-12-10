#include <iostream>
#include <filesystem>
#include "SoundFile.h"

namespace fs = std::filesystem;
using namespace dfx;

constexpr unsigned raw_nChannels = 1;
constexpr auto raw_format = SampleFormat::SINT16;
constexpr double raw_file_rate = 22050.0;

void ScanFile(const std::string_view& fname, bool raw)
{
	SoundFile f;

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
		f.LastError().Print(std::cout);
	}

	std::cout << "Stats for file " << fname << ":\n";
	std::cout << "   Sampling rate:   " << f.fileRate << " Hz" << std::endl;
	std::cout << "   Format:          " << to_string(f.dataType) << std::endl;
	std::cout << "   Channels:        " << f.nChannels << std::endl;
	std::cout << "   Length:          " << f.fileFrames << " frames\n";
	std::cout << "   Duration:        " << f.fileFrames / f.fileRate << " secs\n";

	FrameBuffer<double> buffer(f.fileFrames, f.nChannels);

	bool doNormalize = true;
	f.Read(buffer, 0, 0, doNormalize);

	double window = 0; //  0.100; // in seconds
	double fileRate = f.fileRate;
	auto dataType = f.dataType;

	//auto zebra = ComputeStats(buffer, dataType, fileRate, window);
	auto zebra = ComputeStatsII(buffer, dataType, fileRate, window); // For loudnes

	std::cout << "   Start            " << zebra.start << " (" << (zebra.start / fileRate) << ") secs" << std::endl;
	std::cout << "   End              " << zebra.end << " (" << (zebra.end / fileRate) << ") secs" << std::endl;
	std::cout << "   Neg peak         " << zebra.neg_peak << std::endl;
	std::cout << "   Pos peak         " << zebra.pos_peak << std::endl;
	std::cout << "   Effective bits   " << zebra.effective_bits << std::endl;

	auto mv = maxVal(dataType);

	auto linear_scaled_peak = zebra.peak;
	auto linear_scaled_rms = zebra.rms;

	std::cout << "   Normalized peak  " << linear_scaled_peak << std::endl;
	std::cout << "   Normalized RMS   " << linear_scaled_rms << std::endl;

	auto db_peak = 20.0 * log10(linear_scaled_peak); // 1.0 basis
	auto db_rms = 20.0 * log10(linear_scaled_rms); // 1.0 basis

	std::cout << "   Relative peak    " << db_peak << " dB" << std::endl;
	std::cout << "   Relative rms     " << db_rms << " dB" << std::endl;

	std::cout << "   Est Midi vel     " << int(zebra.rms * 127.0) << std::endl;

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
