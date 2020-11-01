#include "DrumFont.h"
#include "PolyDrummer.h"
#include "DfxMidi.h"
#include "DfxAudio.h"
#include <iostream>

using namespace dfx;

//const char* const ASIO_DRIVER_NAME = "Focusrite USB ASIO";
//const char* const ASIO_DRIVER_NAME = "JRiver Media Center 26";
//const char* const ASIO_DRIVER_NAME = "ReaRoute ASIO (x64)";
const char* const ASIO_DRIVER_NAME = "UMC ASIO Driver";

struct MyData
{
	std::shared_ptr<DfxMidi> inMidi;
	std::shared_ptr<PolyDrummer> poly_drummer;
	MyData() = default;
	MyData(std::shared_ptr<DfxMidi> inMidi_, std::shared_ptr<PolyDrummer> poly_drummer_) 
	: inMidi(inMidi_), poly_drummer(poly_drummer_) 
	{
	};
};

int drumsPlayBack(void* outBuff, void* inBuff, unsigned nFrames, double streamTime, StreamIOStatus ioStatus, void* userData)
{
	// A callback function that plays back whatever sounds are in a polyphonic drumkit

	// WARNING! Runs in a different thread! (The ASIO thread)

	// We don't use streamTime here. Instead, the Drumkit object keeps track of each drum that
	// is being played.

	auto myData = reinterpret_cast<MyData*>(userData);
	auto in_midi = myData->inMidi;
	auto poly_drummer = myData->poly_drummer;

	auto p = reinterpret_cast<system_t*>(outBuff);

	auto m = in_midi->GetMessage(); // Non blocking
	if (m)
	{
		auto tag = m->bytes[0] & 0xf0;

		if (tag == NoteOnMessage::tag)
		{
			auto n_on = in_midi->ParseNoteOn(*m);
			poly_drummer->noteOnDirect(n_on->note, n_on->velocity / 127.0);
			std::cout << '+' << std::flush;
		}
	}

	if (!poly_drummer->HasSoundsToPlay())
	{
		while(nFrames > 0)
		{
			*p++ = 0;
			*p++ = 0;
			--nFrames;
		}

		return 0;
	}

	while (nFrames > 0)
	{
		// NOTE: We are ticking at the playback sampling rate, which may not
		// be the sampling rate of the recorded file. So the tick function
		// below might have to calculate an interpolated frame.
		auto sf = poly_drummer->StereoTick();
		*p++ = sf.left * 0.5;   // @@ TEMP KLUDGE: Apply -6dB of gain to alleviate clipping
		*p++ = sf.right * 0.5;  // @@ TEMP KLUDGE: Apply -6DB of gain to alleviate clipping
		--nFrames;
	}

	return 0;
}


int main()
{
	static constexpr unsigned systemSampleRate = 48000;

	std::string kitFile = "../TestFiles/TestKit.dfx";
	auto df = std::make_unique<DrumFont>();
	auto result = df->LoadFile(std::cout, kitFile);

	if (result != DfxResult::NoError)
	{
		return -1;
	}

	df->drumKits[0]->LoadWaves();

	auto da = MakeAudioApi();

	auto inMidi = MakeInputMidiObject();
	inMidi->ListPorts(std::cout);

	inMidi->OpenPort("AKM320 2");

	auto polyDrummer = std::make_shared<PolyDrummer>();

	polyDrummer->LoadKit(df->drumKits[0]);
	polyDrummer->SetSampleRate(systemSampleRate);

	auto myData = std::make_unique<MyData>(inMidi, polyDrummer);

	// ////////////////////////////////////////////////////////

	bool verbose = true;

	bool b = da->Prep(ASIO_DRIVER_NAME, verbose);
	if (!b)
	{
		std::cout << "Error loading and initializing driver" << std::endl;
		return 0;
	}

	da->ConfigureUserCallback(drumsPlayBack);

	// 0 ins, 2 outs (aka stereo)
	// 64 sample buffers (nominal)

	b = da->Open(0, 2, 64, systemSampleRate, myData.get(), verbose);

	auto start = std::chrono::high_resolution_clock::now();

	if (b)
	{
		b = da->Start();
	}
	else std::cout << "Error opening stream" << std::endl;

	if (b)
	{
		std::cout << "\nAudio session started successfully.\n" << std::endl;

		while (!da->Stopped())
		{
			//fprintf(stdout, "%lf stream time\n", am.getStreamTime());
		}

		auto finish = std::chrono::high_resolution_clock::now();

		auto elapsed = finish - start;
		auto zebra = elapsed.count();

		std::cout << "Session ended. Playing time = " << zebra / 1.0e9 << " secs" << std::endl;
	}
	else std::cout << "Error starting stream" << std::endl;

	return 0;
}

