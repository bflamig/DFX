// MidiTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "RtMidi.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <optional>

struct MidiMessage
{
	std::vector<unsigned char> bytes;
	double stamp;

	MidiMessage(std::vector<unsigned char> bytes_, double stamp_) : bytes(bytes_), stamp(stamp_) {}

	MidiMessage(const MidiMessage& other) : bytes(other.bytes), stamp(other.stamp) {}

	MidiMessage(MidiMessage&& other)
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

	void operator=(MidiMessage&& other)
	{
		if (this != &other)
		{
			bytes = std::move(other.bytes);
			stamp = other.stamp;
			other.stamp = {};
		}
	}
};


struct NoteOnMessage
{
	unsigned channel{};
	unsigned note{};
	unsigned velocity{};

	NoteOnMessage() = default;

	NoteOnMessage(unsigned channel_, unsigned note_, unsigned velocity_)
	: channel(channel_), note(note_), velocity(velocity_)
	{

	}
};

struct NoteOffMessage
{
	unsigned channel{};
	unsigned note{};
	unsigned velocity{};

	NoteOffMessage() = default;

	NoteOffMessage(unsigned channel_, unsigned note_, unsigned velocity_)
	: channel(channel_), note(note_), velocity(velocity_)
	{

	}
};

struct AftertouchMessage
{
	unsigned channel{};
	unsigned note{};
	unsigned pressure{};

	AftertouchMessage() = default;

	AftertouchMessage(unsigned channel_, unsigned note_, unsigned pressure_)
	: channel(channel_), note(note_), pressure(pressure_)
	{

	}
};

struct PitchBendMessage
{
	unsigned channel{};
	double amount{};  // 14 bits

	PitchBendMessage() = default;

	PitchBendMessage(unsigned channel_, double amount_)
	: channel(channel_), amount(amount_)
	{

	}
};

struct ControlChangeMessage
{
	unsigned channel{};
	unsigned controller{};
	unsigned value{};

	ControlChangeMessage() = default;

	ControlChangeMessage(unsigned channel_, unsigned controller_, unsigned value_)
	: channel(channel_), controller(controller_), value(value_)
	{

	}
};


// Higher level wrapper around RtMidi

class DfxMidi {
public:

	std::unique_ptr<RtMidiIn> input_handle;

	std::vector<std::string> inPortNames;

	DfxMidi()
	{
		input_handle = std::make_unique<RtMidiIn>();
	}

	virtual ~DfxMidi()
	{
	}

	void Refresh()
	{
		GatherPortInfo();
	}

	void GatherPortInfo()
	{
		inPortNames = GetPortNames();
	}

	unsigned GetNumPorts()
	{
		return input_handle->getPortCount();
	}

	std::string GetPortName(unsigned port)
	{
		return input_handle->getPortName(port);
	}

	std::vector<std::string> GetPortNames()
	{
		std::vector<std::string> midiNames;
		unsigned nPorts = GetNumPorts();

		for (unsigned i = 0; i < nPorts; i++)
		{
			midiNames.emplace_back(GetPortName(i));
		}

		return midiNames;
	}

	void ListPorts(std::ostream& sout)
	{
		Refresh();

		auto midiNames = inPortNames;

		size_t nPorts = midiNames.size();

		for (size_t i = 0; i < nPorts; i++)
		{
			sout << "  Port: " << i << ":  \"" << midiNames[i] << "\"" << std::endl;
		}

		sout << std::endl;
	}

	void OpenPort(unsigned port)
	{
		input_handle->openPort(port);
	}

	bool OpenPort(const std::string_view& name)
	{
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

	void ListenToAllMessages()
	{
		// Don't ignore sysex, timing, or active sensing messages.
		input_handle->ignoreTypes(false, false, false);
	}

	std::optional<MidiMessage> GetMessage()
	{
		// NOTE: Non-blocking! So don't worry about using this
		// in an interactive loop

		std::vector<unsigned char> messageBytes;

		double stamp = input_handle->getMessage(&messageBytes); // non-blocking

		if (messageBytes.size() > 0)
		{
			return MidiMessage(messageBytes, stamp);
		}
		else return {};
	}

	std::optional<NoteOnMessage> ParseNoteOn(const MidiMessage& m)
	{
		auto& bytes = m.bytes;

		if ((bytes[0] & 0xf0) == 0x90)
		{
			unsigned channel = bytes[0] & 0x0f;
			unsigned note = bytes[1];
			unsigned velocity = bytes[2];
			return NoteOnMessage(channel, note, velocity);
		}
		else return {};
	}

	std::optional<NoteOffMessage> ParseNoteOff(const MidiMessage& m)
	{
		auto& bytes = m.bytes;

		if ((bytes[0] & 0xf0) == 0x80)
		{
			unsigned channel = bytes[0] & 0x0f;
			unsigned note = bytes[1];
			unsigned velocity = bytes[2];
			return NoteOffMessage(channel, note, velocity);
		}
		else return {};
	}

	std::optional<AftertouchMessage> ParseAftertouch(const MidiMessage& m)
	{
		auto& bytes = m.bytes;

		if ((bytes[0] & 0xf0) == 0x80)
		{
			unsigned channel = bytes[0] & 0x0f;
			unsigned note = bytes[1];
			unsigned pressure = bytes[2];
			return AftertouchMessage(channel, note, pressure);
		}
		else return {};
	}

	std::optional<PitchBendMessage> ParsePitchBend(const MidiMessage& m)
	{
		auto& bytes = m.bytes;

		if ((bytes[0] & 0xf0) == 0xe0)
		{
			unsigned channel = bytes[0] & 0x0f;

			// @@ TODO: endian issues?
			int amt = bytes[1] & 0x7f;
			amt |= (bytes[2] & 0x7f) << 7;
			amt -= 0x2000;
			auto amount = (double)amt;

			return PitchBendMessage(channel, amount);
		}
		else return {};
	}

	std::optional <ControlChangeMessage > ParseControlChange(const MidiMessage& m)
	{
		auto& bytes = m.bytes;

		if ((bytes[0] & 0xf0) == 0xb0)
		{
			unsigned channel = bytes[0] & 0x0f;
			unsigned controller = bytes[1];
			unsigned value = bytes[2];
			return ControlChangeMessage(channel, controller, value);
		}
		else return {};
	}


};

void test1()
{
	DfxMidi dm;
	dm.ListPorts(std::cout);
}


void test2()
{
	DfxMidi dm;

	unsigned nPorts = dm.GetNumPorts();

	if (nPorts == 0) 
	{
		std::cout << "No ports available!\n";
		return;
	}

	dm.Refresh();

	dm.OpenPort("AKM320 2");

	// Install an interrupt handler function.
	//done = false;
	//(void)signal(SIGINT, finish);

	// There is an option in VS 2019 to turn off Ctrl-C generating
	// an exception: Debug/Windows/Exception Settings/Win32 Exceptions/ControlC
	// UPDATE: ?? That's not the one. Where is it?

	// Periodically check input queue.
	std::cout << "Reading MIDI from port ... quit with Ctrl-C.\n";

	std::vector<unsigned char> message;
	bool done = false;

	while (!done) 
	{
		auto m = dm.GetMessage();

		if (m)
		{
			std::cout << "deltaT = " << m->stamp << "  ";

			auto n_on = dm.ParseNoteOn(*m);

			if (n_on)
			{
				std::cout << "Note ON: Channel " << n_on->channel << " note " << n_on->note << " vel " << n_on->velocity << std::endl;
				continue;
			}

			auto n_off = dm.ParseNoteOff(*m);

			if (n_off)
			{
				std::cout << "Note OFF: Channel " << n_off->channel << " note " << n_off->note << " vel " << n_off->velocity << std::endl;
				continue;
			}

			auto after = dm.ParseAftertouch(*m);

			if (after)
			{
				std::cout << "Aftertouch: Channel " << after->channel << " note " << after->note << " vel " << after->pressure << std::endl;
				continue;
			}

			auto pitchbend = dm.ParsePitchBend(*m);

			if (pitchbend)
			{
				std::cout << "Pitchbend: Channel " << pitchbend->channel <<  " amount " << pitchbend->amount << std::endl;
				continue;

			}

			auto cc = dm.ParseControlChange(*m);

			if (cc)
			{
				std::cout << "ControlChange: Channel " << cc->channel << " controller " << cc->controller << " val " << cc->value << std::endl;
				continue;
			}


			{
				// Fall back for unknown messages
				auto bytes = m->bytes;
				int nBytes = bytes.size();

				for (int i = 0; i < nBytes; i++)
				{
					std::cout << "Byte " << i << " = " << (int)(bytes[i]) << ", ";
				}

				std::cout << std::endl;
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

	//try
	//{
		test2();
	//}
	//catch (...)
	//{
		//std::cout << "caught exception" << std::endl;
	//}
}

