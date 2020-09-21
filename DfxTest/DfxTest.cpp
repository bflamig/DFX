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
 * This exchange format has a one to one mapping to the widely used Json syntax,
 * simplified to be easier to read and write. It is easy to translate DFX files
 * into Json files that can be processed by any software supporting Json syntax.
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

#include <iostream> 
#include "DfxParser.h"

using namespace bryx;

std::shared_ptr<DfxParser> OpeningCredits()
{
	std::string filename = "../TestFiles/Test1.dfx";

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
	auto df = OpeningCredits();

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
			std::cout << "Dfx schema check VERIFIED." << std::endl;
		}
		else
		{
			std::cout << "Dfx FAILED schema check." << std::endl;
		}

		std::cout << std::endl << std::endl;

		// Might have changed string tokens to number tokens. Let's see.
		df->WriteDfx(std::cout, SyntaxModeEnum::Bryx);

	}

	return 0;
}

int main()
{
	explore();
	return 0;
}
