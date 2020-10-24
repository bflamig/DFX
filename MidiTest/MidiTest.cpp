// MidiTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "RtMidi.h"
#include <iostream>
#include <chrono>
#include <thread>

std::vector<std::string> enumeratePorts()
{
	auto inMidi = std::make_unique<RtMidiIn>();

	unsigned nPorts = inMidi->getPortCount();

	std::vector<std::string> midiNames(nPorts);

	for (unsigned i = 0; i < nPorts; i++)
	{
		auto portName = inMidi->getPortName(i);
		midiNames[i] = portName;
	}

	return midiNames;
}

void test1()
{
	auto midiNames = enumeratePorts();

	auto nPorts = midiNames.size();

	for (unsigned i = 0; i < nPorts; i++)
	{
		std::cout << "  Port: " << i << ":  \"" << midiNames[i] << "\"\n";
	}

	std::cout << std::endl;
}


#if 0
void midiErrorCallback(RtMidiError::Type type, const std::string& errorText, void* userData)
{
}

void midiCallback(double timeStamp, std::vector<unsigned char> *message, void* userData)
{

}

void test2()
{
	auto inMidi = std::make_unique<RtMidiIn>();

	inMidi->setErrorCallback(midiErrorCallback);
	inMidi->setCallback(midiCallback);

	inMidi->openPort(0);
}

#endif

void test2()
{
	auto midiin = std::make_unique<RtMidiIn>();

	// Check available ports.
	unsigned int nPorts = midiin->getPortCount();

	if (nPorts == 0) 
	{
		std::cout << "No ports available!\n";
		return;
	}

	midiin->openPort(0);

	// Don't ignore sysex, timing, or active sensing messages.
	// midiin->ignoreTypes(false, false, false);

	// Install an interrupt handler function.
	//done = false;
	//(void)signal(SIGINT, finish);

	// Periodically check input queue.
	std::cout << "Reading MIDI from port ... quit with Ctrl-C.\n";

	std::vector<unsigned char> message;
	bool done = false;

	while (!done) 
	{
		double stamp = midiin->getMessage(&message); // non-blocking
		int nBytes = message.size();

		for (int i = 0; i < nBytes; i++)
		{
			std::cout << "Byte " << i << " = " << (int)message[i] << ", ";
		}

		if (nBytes > 0)
		{
			std::cout << "stamp = " << stamp << std::endl;
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

