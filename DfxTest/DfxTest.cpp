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

#include <iostream> 
#include "DfxParser.h"

using namespace dfx;

int TestDfx()
{
	// Test a full dfx file.

	//std::string_view filename = "../TestFiles/Test1.dfx";
	//std::string_view filename = "../TestFiles/Test1NC.dfx";
	//std::string_view filename = "../TestFiles/TestKit.dfx";
	std::string_view filename = "../TestFiles/TestKitWIncludes.dfx";

	auto dp = std::make_unique<DfxParser>();

	auto rv = dp->LoadAndVerify(std::cout, filename);

	if (rv == DfxResult::NoError)
	{
		std::cout << "Dfx schema check VERIFIED." << std::endl;
		auto num_kits = dp->NumKits();
		std::cout << "num kits = " << num_kits << std::endl;

		std::cout << std::endl << std::endl;

		// Might have changed string tokens to number tokens. Let's see.
		dp->WriteDfx(std::cout, SyntaxModeEnum::Bryx);

		std::cout << std::endl << std::endl;

		// Let's see what we look like as json

		std::cout << "In Json syntax" << std::endl << std::endl;

		dp->WriteDfx(std::cout, SyntaxModeEnum::Json);
	}
	else
	{
		std::cout << "Dfx FAILED schema check." << std::endl;
	}

	return 0;
}

int TestDfxi()
{
	// Test an dfx instrument include file
	std::string_view filename = "G:/DrumSW/WaveLibrary/DownloadedWaves/FocusRite/snare.dfxi";

	auto dp = std::make_unique<DfxParser>();

	bool as_include = true;
	auto rv = dp->LoadAndVerify(std::cout, filename, as_include);

	if (rv == DfxResult::NoError)
	{
		std::cout << "Dfxi schema check VERIFIED." << std::endl;
		auto num_kits = dp->NumKits();
		std::cout << "num kits = " << num_kits << std::endl;

		std::cout << std::endl << std::endl;

		// Might have changed string tokens to number tokens. Let's see.
		dp->WriteDfx(std::cout, SyntaxModeEnum::Bryx);

		std::cout << std::endl << std::endl;

		// Let's see what we look like as json

		std::cout << "In Json syntax" << std::endl << std::endl;

		dp->WriteDfx(std::cout, SyntaxModeEnum::Json);
	}
	else
	{
		std::cout << "Dfxi FAILED schema check." << std::endl;
	}

	return 0;
}

int main()
{
	TestDfx();
	//TestDfxi();
	return 0;
}
