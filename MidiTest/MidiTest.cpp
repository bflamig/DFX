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

			auto bytes = m->bytes;
			int nBytes = bytes.size();

			for (int i = 0; i < nBytes; i++)
			{
				std::cout << "Byte " << i << " = " << (int)(bytes[i]) << ", ";
			}

			std::cout << std::endl;

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

