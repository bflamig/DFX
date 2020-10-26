#pragma once

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

#include <vector>
#include <string>
#include <optional>
#include <ostream>

namespace dfx
{

	struct MidiMessage
	{
		std::vector<uint8_t> bytes;
		double stamp;

		MidiMessage(std::vector<uint8_t> bytes_, double stamp_) : bytes(bytes_), stamp(stamp_) {}

		MidiMessage(const MidiMessage& other) : bytes(other.bytes), stamp(other.stamp) {}

		MidiMessage(MidiMessage&& other) noexcept
			: bytes(std::move(other.bytes))
			, stamp(other.stamp)
		{
			other.stamp = {};
		}

		void operator=(const MidiMessage& other)
		{
			if (this != &other)
			{
				bytes = other.bytes;
				stamp = other.stamp;
			}
		}

		void operator=(MidiMessage&& other) noexcept
		{
			if (this != &other)
			{
				bytes = std::move(other.bytes);
				stamp = other.stamp;
				other.stamp = {};
			}
		}
	};


	struct NoteOffMessage
	{
		static constexpr uint8_t tag = 0x80;

		uint8_t channel{};
		uint8_t note{};
		uint8_t velocity{};

		NoteOffMessage() = default;

		NoteOffMessage(uint8_t channel_, uint8_t note_, uint8_t velocity_)
			: channel(channel_), note(note_), velocity(velocity_)
		{

		}
	};

	struct NoteOnMessage
	{
		static constexpr uint8_t tag = 0x90;

		uint8_t channel{};
		uint8_t note{};
		uint8_t velocity{};

		NoteOnMessage() = default;

		NoteOnMessage(uint8_t channel_, uint8_t note_, uint8_t velocity_)
			: channel(channel_), note(note_), velocity(velocity_)
		{

		}
	};


	struct AftertouchMessage
	{
		static constexpr uint8_t tag = 0xa0;

		uint8_t channel{};
		uint8_t note{};
		uint8_t pressure{};

		AftertouchMessage() = default;

		AftertouchMessage(uint8_t channel_, uint8_t note_, uint8_t pressure_)
			: channel(channel_), note(note_), pressure(pressure_)
		{

		}
	};

	struct ControlChangeMessage
	{
		static constexpr uint8_t tag = 0xb0;

		uint8_t channel{};
		uint8_t controller{};
		uint8_t value{};

		ControlChangeMessage() = default;

		ControlChangeMessage(uint8_t channel_, uint8_t controller_, uint8_t value_)
			: channel(channel_), controller(controller_), value(value_)
		{

		}
	};

	struct ProgramChangeMessage
	{
		static constexpr uint8_t tag = 0xc0;

		uint8_t channel{};
		uint8_t new_program{};

		ProgramChangeMessage() = default;

		ProgramChangeMessage(uint8_t channel_, uint8_t new_program_)
			: channel(channel_), new_program(new_program_)
		{

		}
	};


	struct ChannelAftertouchMessage
	{
		static constexpr uint8_t tag = 0xd0;

		uint8_t channel{};
		uint8_t pressure{};

		ChannelAftertouchMessage() = default;

		ChannelAftertouchMessage(uint8_t channel_, uint8_t pressure_)
			: channel(channel_), pressure(pressure_)
		{

		}
	};

	struct PitchBendMessage
	{
		static constexpr uint8_t tag = 0xe0;

		uint8_t channel{};
		double amount{};  // 14 bits

		PitchBendMessage() = default;

		PitchBendMessage(uint8_t channel_, double amount_)
			: channel(channel_), amount(amount_)
		{

		}
	};

	struct SystemMessage
	{
		static constexpr uint8_t tag = 0xf0;

		std::vector<uint8_t> bytes;

		SystemMessage()
		{
			bytes.push_back(tag);
		}

		SystemMessage(const std::vector<uint8_t>& bytes_)
			: bytes(bytes_)
		{

		}
	};


	// ///////////////////////////////////////

	// Abstract Midi interface that hides a 
	// lot of the details

	class DfxMidi {
	public:

		std::vector<std::string> inPortNames;

	public:

		DfxMidi() = default;
		virtual ~DfxMidi() { }

		virtual void ScanPorts() = 0;
		virtual unsigned GetNumPorts() = 0;

		virtual std::string GetPortName(unsigned port) = 0;
		std::vector<std::string> GetPortNames();

		void ListPorts(std::ostream& sout);

		virtual void OpenPort(unsigned port) = 0;
		bool OpenPort(const std::string_view& name); // Must call ScanPorts() ahead of time

		virtual void ListenToAllMessages() = 0;
		virtual std::optional<MidiMessage> GetMessage() = 0;

		std::optional<NoteOffMessage> ParseNoteOff(const MidiMessage& m);
		std::optional<NoteOnMessage> ParseNoteOn(const MidiMessage& m);
		std::optional<AftertouchMessage> ParseAftertouch(const MidiMessage& m);
		std::optional<ControlChangeMessage> ParseControlChange(const MidiMessage& m);
		std::optional<ProgramChangeMessage> ParseProgramChange(const MidiMessage& m);
		std::optional<ChannelAftertouchMessage> ParseChannelAfterTouch(const MidiMessage& m);
		std::optional<PitchBendMessage> ParsePitchBend(const MidiMessage& m);
		std::optional<SystemMessage> ParseSystemMessage(const MidiMessage& m);
	};

	extern std::shared_ptr<DfxMidi> MakeInputMidiObject();

} // end of namespace