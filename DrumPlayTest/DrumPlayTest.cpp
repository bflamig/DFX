#include "DrumFont.h"
#include "DfxMidi.h"
#include "DfxAudio.h"
#include "AsioMgr.h"
#include <iostream>

using namespace dfx;

int main()
{
#if 0
	std::string kitFile = "../TestFiles/TestKit.dfx";
	auto df = std::make_unique<DrumFont>();
	auto result = df->LoadFile(std::cout, kitFile);

	AsioMgr da;
	bool b = 


	auto inMidi = MakeInputMidiObject();
	inMidi->ListPorts(std::cout);

	inMidi->OpenPort("AKM320 2");

#endif


}

