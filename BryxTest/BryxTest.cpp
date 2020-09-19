
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
#include "BryxParser.h"

using namespace bryx;

bool lexi_test(const std::string z, SyntaxModeEnum smode = SyntaxModeEnum::AutoDetect)
{
	std::stringstream fred{ z };

	Lexi sun(fred.rdbuf());

	sun.SetSyntaxMode(smode);

	auto t = sun.Start();

	while (!IndicatesQuit(t.type))
	{
		t = sun.Next();
	}

	auto rv = sun.last_lexical_error.code;


	if (rv == LexiResult::NoError)
	{
		std::cout << "Lexical scan PASS" << std::endl;
		return true;
	}
	else
	{
		std::cout << "Lexical scan FAIL: " << std::endl;
		sun.last_lexical_error.Print(std::cout);
		return false;
	}
}

bool parser_test(std::string z, bool dfx_mode, SyntaxModeEnum smode = SyntaxModeEnum::AutoDetect)
{
	std::stringstream fred{ z };

	Parser parser(fred.rdbuf());

	parser.SetSyntaxMode(smode);

	parser.SetDfxMode(dfx_mode);

	auto rv = parser.Parse();

	if (rv == ParserResult::NoError)
	{
		std::cout << "PARSER PASS" << std::endl;
	}
	else
	{
		std::cout << "PARSER FAIL: ";
		parser.last_parser_error.Print(std::cout); // @@ TODO: Make sure this is filled when needed. It's not at the moment.
	}

	return rv == ParserResult::NoError;
}

using TestDataType = std::pair<int, const std::string>;

std::vector<TestDataType> test_data
{
	{1, "{}"},
	{2, "[]"},
	{3, "42"},
	{4, "4.2"},
	{5, "abc"},
	{6, R"("abc")"},
	{7, "x = y"},
	{8, "{x = y}"},
	{9, "[x = y]"},
	{10, R"("dfx" = "val")"},
	{11, R"(dfx = "val")"},
	{12, R"({[]})"},  // EXPECTED TO FAIL
	{13, R"([{}])"},
	{14, R"([{}, {}])"},
	{15, R"(dfx = { mydrumKit = { path = "", stuff = {} }})"},
	{16, R"("dfx" = { mydrumKit = { path = "", stuff = {} }})"},
	{17, R"("dfx" = { mydrumKit = { path = "fred/abc", stuff = {} }})"},
	{18, R"("dfx" = { mydrumKit = { path = fred/abc, stuff = {} }})"}

};

void LexiTest(SyntaxModeEnum syntax_mode = SyntaxModeEnum::AutoDetect)
{
	for (auto& z : test_data)
	{
		std::cout << z.first << ": ";
		lexi_test(z.second, syntax_mode);
	}
}

void ParserTest(bool dfx_mode = true, SyntaxModeEnum syntax_mode = SyntaxModeEnum::AutoDetect)
{
	for (auto& z : test_data)
	{
		std::cout << z.first << ": ";
		parser_test(z.second, dfx_mode, syntax_mode);
	}
}

void ParserTest(int i, bool dfx_mode = true, SyntaxModeEnum syntax_mode = SyntaxModeEnum::AutoDetect)
{
	std::cout << i << ": ";
	parser_test(test_data[i-1].second, dfx_mode, syntax_mode);  // Must use base 1 here
}

int main()
{
    std::cout << "Lexical tests\n";

	LexiTest(SyntaxModeEnum::Bryx);

	std::cout << "\nParser tests\n";

	bool dfx_mode = false;
	ParserTest(dfx_mode, SyntaxModeEnum::Bryx);

	//ParserTest(12, dfx_mode, SyntaxModeEnum::Bryx);

}

