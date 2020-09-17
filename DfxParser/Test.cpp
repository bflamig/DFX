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
 * This exchange format has a nested syntax with a one to one mapping to the
 * Json syntax widely used on the web, but simplified to be easier to read and
 * write. Because of the one-to-one mapping, it's easy to translate DFX files
 * into Json files that can be processed by any software that supports Json
 * syntax.
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


// DfxTest.cpp : This file contains the 'main' function. Program execution begins
// and ends there.

#include <iostream> 
#include "DfxParser.h"

using namespace bryx;

std::shared_ptr<DfxParser> OpeningCredits(SyntaxModeEnum syntax_mode)
{
	std::string filename = "../TestFiles/Test1.dfx";

	//if (syntax_mode == SyntaxModeEnum::Bryx)
	//{
	//	filename = "C:/users/bryan/onedrive/documents/Test1.dfx";
	//}
	//else filename = "C:/users/bryan/onedrive/documents/DrumFont2.json";

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
	auto df = OpeningCredits(SyntaxModeEnum::Json);

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
