/******************************************************************************\
 * DFX - "Drum font exchange format" - source code
 *
 * Copyright (c) 2020 by Bryan Flamig
 *
 * This software helps facilitate the real-time playing of multi-layered drum
 * samples, by implementing a language that specifies a master directory of the
 * the sample wave files for a drum kit: where the samples are, what they are
 * for, and a summary of their properties. The DFX format allows modifications
 * of sample levels to achieve a unified, pleasing mix of sounds. Mechanisms
 * such as velocity layers and round robins are supported for this purpose.
 *
 * This exchange format has a one to one mapping to the widely used Json syntax,
 * simplified to be easier to read and write. It is easy to translate DFX files
 * into Json files that can be parsed by any software supporting Json syntax.
 *
 ******************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
\******************************************************************************/

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

struct PlaybackData
{
	std::shared_ptr<DfxMidi> midi_input;
	std::shared_ptr<PolyDrummer> poly_drummer;
	PlaybackData() = default;
	PlaybackData(std::shared_ptr<DfxMidi> midi_input_, std::shared_ptr<PolyDrummer> poly_drummer_) 
	: midi_input(midi_input_), poly_drummer(poly_drummer_) 
	{
	};
};


int ProcessMidi(DfxMidi* midi_input, PolyDrummer* poly_drummer)
{
	auto m = midi_input->GetMessage(); // Non blocking
	if (m)
	{
		auto tag = m->bytes[0] & 0xf0;

		if (tag == NoteOnMessage::tag)
		{
			auto n_on = midi_input->ParseNoteOn(*m);
			poly_drummer->noteOnDirect(n_on->note, n_on->velocity); // / 127.0);
			//std::cout << '+' << std::flush;
			std::cout << int(n_on->velocity) << ' ' << std::flush;
		}
	}

	return 0;
}

int DrumsPlayBack(void* outBuff, void* inBuff, unsigned nFrames, double streamTime, StreamIOStatus ioStatus, void* userData)
{
	// A callback function that plays back a buffer's worth of whatever sounds are active in our polyphonic drumkit

	// WARNING! Runs in a different thread! (The ASIO thread)

	// We don't use streamTime here. Instead, the PolyDrummer object keeps track of each drum that
	// is being played.

	auto p = reinterpret_cast<system_t*>(outBuff);

	auto playbackData = reinterpret_cast<PlaybackData*>(userData);
	auto midi_input = playbackData->midi_input;
	auto poly_drummer = playbackData->poly_drummer;

	// We do input midi processing right in this callback. This makes things simpler
	// from a thread synchronization standpoint. We don't have to do any locking here.

	// We look for midi messages every nn times through processing samples here.
	// So if our buffer is 64 samples, this means we look for midi messages 64/nn 
	// times, for each call to this playback. This helps keep the midi latency low
	// without using undue checking.

	static constexpr unsigned outer_loop_chunk = 16;

	if (poly_drummer->HasSoundsToPlay())
	{
		while (nFrames > 0)
		{
			// An outer loop call:
			ProcessMidi(midi_input.get(), poly_drummer.get()); // Non-blocking.

			auto num_to_do = outer_loop_chunk <= nFrames ? outer_loop_chunk : nFrames;
			nFrames -= num_to_do;

			// inner loop

			while (num_to_do > 0)
			{
				// NOTE: We are ticking at the playback sampling rate, which may not
				// be the sampling rate of the recorded file. So the tick function
				// below might have to calculate an interpolated frame.
				auto sf = poly_drummer->StereoTick();
				// @@ TODO: Apply volume gain from midi volume control or gui control or whatever.
				*p++ = sf.left * 0.5;   // @@ TEMP KLUDGE: Apply -6dB of gain to alleviate clipping
				*p++ = sf.right * 0.5;  // @@ TEMP KLUDGE: Apply -6DB of gain to alleviate clipping
				--num_to_do;
			}
		}
	}
	else
	{
		// All drums quiet

		while (nFrames > 0)
		{
			// An outer loop call:
			ProcessMidi(midi_input.get(), poly_drummer.get()); // Non-blocking.

			auto num_to_do = outer_loop_chunk <= nFrames ? outer_loop_chunk : nFrames;
			nFrames -= num_to_do;

			// Play silence in inner  loop

			while (num_to_do > 0)
			{
				*p++ = 0;
				*p++ = 0;
				--num_to_do;
			}
		}
	}

	return 0;
}


int main()
{
	//
	// Using 48kHz sample rate is a reasonable choice.
	//

	static constexpr unsigned systemSampleRate = 48000;

	//
	// Setup the input midi controller
	//

	auto inMidi = MakeInputMidiObject();

	std::cout << "Available input midi ports:" << std::endl;
	inMidi->ListPorts(std::cout);

	// For our test we'll use a small midi keyboard

	//bool b1 = inMidi->OpenPort("AKM320");
	bool b1 = inMidi->OpenPort("UMC404HD 192k MIDI In");

	if (b1)
	{
		std::cout << "Input midi port successfully opened" << std::endl;
	}
	else
	{
		std::cout << "FAILURE opening input midi port" << std::endl;
		return -1;
	}

	//
	// Load the drum font file (and verify it and build drum kit)
	//

	//std::string_view dfxile = "../TestFiles/TestKit.dfx";
	//std::string_view dfxFile = "../TestFiles/Tabla.dfx";
	//std::string_view dfxFile = "W:/reaper/ExpDrum.dfx";
	std::string_view dfxFile = "W:/reaper/Jungle/JungleDrums.dfx";
	//std::string_view dfxFile = "W:/reaper/Tabla/TablaDrums.dfx";

	auto df = std::make_unique<DrumFont>();

	// @@ TODO: Error handling still a jumbled mess.

	auto result = df->LoadFile(std::cout, dfxFile);
	if (result != DfxResult::NoError)
	{
		// Error message already printed to std::cout
		return -1;
	}

	//
	// Load all the corresponding wave samples for the drum 
	// font into memory. This might take a while.
	//

	std::cout << "Loading all drum font wave files. This may take awhile ..." << std::endl;

	int errcnt = df->drumKits[0]->LoadWaves(std::cout);

	if (errcnt != 0)
	{
		// Note: individual error messages already printed out
		std::cout << "Stopping due to " << errcnt << " file loading error(s)." << std::endl;
		return -1;
	}

	//
	// Setup up our polyphonic drum player
	//

	auto polyDrummer = std::make_shared<PolyDrummer>();

	// The drum font can store multiple kits, but for now we're
	// only going to use the first one we find.

	polyDrummer->UseKit(df->drumKits[0], systemSampleRate);

	auto playbackData = std::make_unique<PlaybackData>(inMidi, polyDrummer);

	//
	// Open the audio playback device session
	//

	bool verbose = true;

	auto da = MakeAudioApi();

	bool b = da->Prep(ASIO_DRIVER_NAME, verbose);
	if (!b)
	{
		std::cout << "Error loading and initializing audio driver" << std::endl;
		return 0;
	}

	da->ConfigureUserCallback(DrumsPlayBack);

	// Start playback audio stream
	// 0 ins, 2 outs (aka stereo)
	// 64 sample buffers (nominal)

	b = da->Open(0, 2, 64, systemSampleRate, playbackData.get(), verbose);

	auto start = std::chrono::high_resolution_clock::now();

	// Now we can finally start the playback audio session.

	if (b)
	{
		b = da->Start(); // start the audio session
	}
	else std::cout << "Error opening audio stream" << std::endl;

	if (b)
	{
		std::cout << "\nAudio session started successfully.\n" << std::endl;

		// We let the ASIO driver run the show now, until we receive a signal
		// to stop. All we have to do here is wait for the signal.
		// @@TODO: Make this more efficient than polling. Perhaps arrange a
		// wait for object here.

		while (!da->Stopped())
		{
			//fprintf(stdout, "%lf stream time\n", am.getStreamTime());
		}

		auto finish = std::chrono::high_resolution_clock::now();

		auto elapsed = finish - start;
		auto zebra = elapsed.count();

		std::cout << "Session ended. Playing time = " << zebra / 1.0e9 << " secs" << std::endl;
	}
	else std::cout << "Error starting audio session" << std::endl;

	return 0;
}

