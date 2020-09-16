// DfxTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <iostream> 
#include "DfxParser.h"

using namespace bryx;

std::shared_ptr<DfxParser> OpeningCredits(SyntaxModeEnum syntax_mode)
{
	std::string filename;

	if (syntax_mode == SyntaxModeEnum::Bryx)
	{
		filename = "C:/users/bryan/onedrive/documents/DrumFont2.bryx";
	}
	else filename = "C:/users/bryan/onedrive/documents/DrumFont2.json";

	auto df = std::make_shared<DfxParser>();

	auto result = df->LoadFile(filename);

	if (result == ParserResult::FileOpenError)
	{
		std::cout << "failed to open: " << filename << std::endl;
	}
	else if (result == ParserResult::CannotDetermineSyntaxMode)
	{
		std::cout << "cannot determine syntax mode: " << filename << std::endl;
	}
	else if (result != ParserResult::NoError)
	{
		std::cout << "Parsing error encountered:\n";
		df->PrintError(std::cout);
	}
	else
	{
		df->WriteDfx(std::cout, SyntaxModeEnum::Bryx);
		std::cout << std::endl;
		return df;
	}

	return nullptr;
}

int explore()
{
	auto df = OpeningCredits(SyntaxModeEnum::Bryx);

	if (df != nullptr)
	{
		auto num_kits = df->NumKits();
		std::cout << "num kits = " << num_kits << std::endl;

		using kits_map = bryx::object_map_type;

		df->StartLog(std::cout);
		auto zebra = df->Verify();
		df->EndLog();

		if (zebra)
		{
			std::cout << "Drum font schema check VERIFIED." << std::endl;
		}
		else
		{
			std::cout << "Drum font FAILED schema check." << std::endl;
		}
	}

	return 0;
}

int main()
{
	explore();
	return 0;
}
