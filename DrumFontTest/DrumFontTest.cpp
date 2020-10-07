
#include <iostream>
#include "DrumFont.h"

using namespace bryx;

void test()
{
	std::string filename = "../TestFiles/Test1.dfx";
	auto df = std::make_unique<DrumFont>();
	auto result = df->LoadFile(std::cout, filename);

	std::cout << std::endl << "List of robin paths" << std::endl;
	df->DumpRobins(std::cout);
}

int main()
{
	test();
}

