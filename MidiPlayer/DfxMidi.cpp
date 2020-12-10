
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

#include "RtMidi.h"

namespace dfx
{

	std::vector<std::string> DfxMidi::GetPortNames()
	{
		std::vector<std::string> midiNames;
		unsigned nPorts = GetNumPorts();

		for (unsigned i = 0; i < nPorts; i++)
		{
			midiNames.emplace_back(GetPortName(i));
		}

		return midiNames;
	}


	void DfxMidi::ListPorts(std::ostream& sout)
	{
		// Must call ScanPorts first!

		ScanPorts();

		auto midiNames = inPortNames;

		size_t nPorts = midiNames.size();

		for (size_t i = 0; i < nPorts; i++)
		{
			sout << "  Port: " << i << ":  \"" << midiNames[i] << "\"" << std::endl;
		}

		sout << std::endl;
	}

	bool DfxMidi::OpenPort(const std::string_view& name)
	{
		// Should have called ScanPorts() first

		unsigned p = 0;
		for (auto& n : inPortNames)
		{
			if (n == name)
			{
				OpenPort(p);
				return true;
			}

			++p;
		}

		return false;
	}

	std::optional<NoteOffMessage> DfxMidi::ParseNoteOff(const MidiMessage& m)
	{
		auto& bytes = m.bytes;

		if ((bytes[0] & 0xf0) == NoteOffMessage::tag)
		{
			uint8_t channel = bytes[0] & 0x0f;
			uint8_t note = bytes[1] & 0x7f;
			uint8_t velocity = bytes[2] & 0x7f;
			return NoteOffMessage(channel, note, velocity);
		}
		else return {};
	}

	std::optional<NoteOnMessage> DfxMidi::ParseNoteOn(const MidiMessage& m)
	{
		auto& bytes = m.bytes;

		if ((bytes[0] & 0xf0) == NoteOnMessage::tag)
		{
			uint8_t channel = bytes[0] & 0x0f;
			uint8_t note = bytes[1] & 0x7f;
			uint8_t velocity = bytes[2] & 0x7f;
			return NoteOnMessage(channel, note, velocity);
		}
		else return {};
	}

	std::optional<AftertouchMessage> DfxMidi::ParseAftertouch(const MidiMessage& m)
	{
		auto& bytes = m.bytes;

		if ((bytes[0] & 0xf0) == AftertouchMessage::tag)
		{
			uint8_t channel = bytes[0] & 0x0f;
			uint8_t note = bytes[1] & 0x7f;
			uint8_t pressure = bytes[2] & 0x7f;
			return AftertouchMessage(channel, note, pressure);
		}
		else return {};
	}

	std::optional <ControlChangeMessage> DfxMidi::ParseControlChange(const MidiMessage& m)
	{
		auto& bytes = m.bytes;

		if ((bytes[0] & 0xf0) == ControlChangeMessage::tag)
		{
			uint8_t channel = bytes[0] & 0x0f;
			uint8_t controller = bytes[1] & 0x7f;
			uint8_t value = bytes[2] & 0x7f;
			return ControlChangeMessage(channel, controller, value);
		}
		else return {};
	}

	std::optional <ProgramChangeMessage> DfxMidi::ParseProgramChange(const MidiMessage& m)
	{
		auto& bytes = m.bytes;

		if ((bytes[0] & 0xf0) == ProgramChangeMessage::tag)
		{
			uint8_t channel = bytes[0] & 0x0f;
			uint8_t new_program = bytes[1] & 0x7f;
			return ProgramChangeMessage(channel, new_program);
		}
		else return {};
	}

	std::optional <ChannelAftertouchMessage> DfxMidi::ParseChannelAfterTouch(const MidiMessage& m)
	{
		auto& bytes = m.bytes;

		if ((bytes[0] & 0xf0) == ChannelAftertouchMessage::tag)
		{
			uint8_t channel = bytes[0] & 0x0f;
			uint8_t pressure = bytes[1] & 0x7f;
			return ChannelAftertouchMessage(channel, pressure);
		}
		else return {};
	}

	std::optional<PitchBendMessage> DfxMidi::ParsePitchBend(const MidiMessage& m)
	{
		auto& bytes = m.bytes;

		if ((bytes[0] & 0xf0) == PitchBendMessage::tag)
		{
			uint8_t channel = bytes[0] & 0x0f;

			// @@ TODO: endian issues?
			int amt = bytes[1] & 0x7f;
			amt |= (bytes[2] & 0x7f) << 7;
			amt -= 0x2000;
			auto amount = (double)amt;

			return PitchBendMessage(channel, amount);
		}
		else return {};
	}

	std::optional<SystemMessage> DfxMidi::ParseSystemMessage(const MidiMessage& m)
	{
		auto& bytes = m.bytes;

		if ((bytes[0] & 0xf0) == SystemMessage::tag)
		{
			return SystemMessage(m.bytes);
		}
		else return {};
	}


	// ///////////////////////////////////////////////////////////
	//
	// Higher level wrapper around RtMidi
	//
	// ///////////////////////////////////////////////////////////

	class DfxMidiRt : public DfxMidi {
	public:

		std::unique_ptr<RtMidiIn> input_handle;

		DfxMidiRt()
			: DfxMidi{}
		{
			input_handle = std::make_unique<RtMidiIn>();
		}

		virtual ~DfxMidiRt()
		{
		}

		virtual void ScanPorts()
		{
			inPortNames = GetPortNames();
		}

		virtual unsigned GetNumPorts()
		{
			return input_handle->getPortCount();
		}

		virtual std::string GetPortName(unsigned port)
		{
			return input_handle->getPortName(port);
		}

		virtual void OpenPort(unsigned port)
		{
			input_handle->openPort(port);
		}

		virtual void ListenToAllMessages()
		{
			// Don't ignore sysex, timing, or active sensing messages.
			input_handle->ignoreTypes(false, false, false);
		}

		virtual std::optional<MidiMessage> GetMessage()
		{
			// NOTE: Non-blocking! So don't worry about using this
			// in an interactive loop

			std::vector<uint8_t> messageBytes;

			double stamp = input_handle->getMessage(&messageBytes); // non-blocking

			if (messageBytes.size() > 0)
			{
				return MidiMessage(messageBytes, stamp);
			}
			else return {};
		}

	};


	std::shared_ptr<DfxMidi> MakeInputMidiObject()
	{
		return std::make_shared<DfxMidiRt>();
	}

} // end of namespace