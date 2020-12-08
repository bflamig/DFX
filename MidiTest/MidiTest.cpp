/******************************************************************************\
 * Bryx - "Bryan exchange format" - source code
 *
 * Copyright (c) 2020 by Bryan Flamig
 *
 * This exchange format has a one to one mapping to the widely used Json syntax,
 * simplified to be easier to read and write. It is easy to translate Bryx files
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


#include "DfxMidi.h"
#include <iostream>
#include <chrono>
#include <thread>

using namespace dfx;

void test1()
{
	auto dm = MakeInputMidiObject();
	dm->ListPorts(std::cout);
}


void test2()
{
	auto dm = MakeInputMidiObject();

	dm->ScanPorts();

	//dm->OpenPort("AKM320 2");
	dm->OpenPort("2- UMC404HD 192k MIDI In 0");

	// There is an option in VS 2019 to turn off Ctrl-C generating
	// an exception: Debug/Windows/Exception Settings/Win32 Exceptions/ControlC
	// UPDATE: ?? That's not the one. Where is it? UPDATE: It's the one, I think.

	// Periodically check input queue.
	std::cout << "Reading MIDI from port ... quit with Ctrl-C.\n";

	std::vector<unsigned char> message;
	bool done = false;

	while (!done) 
	{
		auto m = dm->GetMessage();

		if (m)
		{
			std::cout << "dT = " << m->stamp << "  ";

			auto tag = m->bytes[0] & 0xf0;

			switch (tag)
			{
				case NoteOffMessage::tag:
				{
					auto n_off = dm->ParseNoteOff(*m);
					if (n_off)
					{
						std::cout << "Note OFF: Channel " << int(n_off->channel) << " note " << int(n_off->note) << " vel " << int(n_off->velocity) << std::endl;
					}
				}
				break;

				case NoteOnMessage::tag:
				{
					auto n_on = dm->ParseNoteOn(*m);

					if (n_on)
					{
						std::cout << "Note ON: Channel " << int(n_on->channel) << " note " << int(n_on->note) << " vel " << int(n_on->velocity) << std::endl;
					}
				}
				break;

				case AftertouchMessage::tag:
				{
					auto after = dm->ParseAftertouch(*m);

					if (after)
					{
						std::cout << "Aftertouch: Channel " << int(after->channel) << " note " << int(after->note) << " vel " << int(after->pressure) << std::endl;
					}
				}
				break;

				case ControlChangeMessage::tag:
				{
					auto cc = dm->ParseControlChange(*m);

					if (cc)
					{
						std::cout << "ControlChange: Channel " << int(cc->channel) << " controller " << int(cc->controller) << " val " << int(cc->value) << std::endl;
					}
				}
				break;

				case ProgramChangeMessage::tag:
				{
					auto pc = dm->ParseProgramChange(*m);

					if (pc)
					{
						std::cout << "ProgramChange: Channel " << int(pc->channel) << " new program " << int(pc->new_program) << std::endl;
					}
				}
				break;

				case ChannelAftertouchMessage::tag:
				{
					auto cat = dm->ParseChannelAfterTouch(*m);

					if (cat)
					{
						std::cout << "ChannelAftertouch: Channel " << int(cat->channel) << " pressure " << int(cat->pressure) << std::endl;
					}
				}
				break;

				case PitchBendMessage::tag:
				{
					auto pb = dm->ParsePitchBend(*m);

					if (pb)
					{
						std::cout << "Pitchbend: Channel " << int(pb->channel) << " amount " << pb->amount << std::endl;
					}
				}
				break;

				case SystemMessage::tag:
				{
					auto sm = dm->ParseSystemMessage(*m);

					if (sm)
					{
						std::cout << "System message: "; 
						auto bytes = m->bytes;
						int nBytes = bytes.size();

						for (int i = 0; i < nBytes; i++)
						{
							std::cout << "Byte " << i << " = " << int(bytes[i]) << ", ";

						}
					}
				}
				break;

				default:
				{
					// Fall back for unknown messages

					auto bytes = m->bytes;
					int nBytes = bytes.size();

					std::cout << "Unknown message: ";

					for (int i = 0; i < nBytes; i++)
					{
						std::cout << "Byte " << i << " = " << int(bytes[i]) << ", ";
					}

					std::cout << std::endl;
				}
			}
		}

		// Sleep for 10 milliseconds ... platform-dependent.

		using namespace std::chrono_literals;
		std::this_thread::sleep_for(10ms);
	}
}

int main()
{
	test1();
	test2();
}

