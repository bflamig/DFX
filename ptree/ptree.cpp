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
#include <vector>
#include <string_view>
#include "SymTree.h"
#include "Units.h"

using namespace bryx;

void test1()
{
	// Test the generic sym tree

	auto tp = SymTree();

	tp.addkey("abc", 42);
	tp.print(std::cout);
	std::cout << std::endl << std::endl;

	tp.addkey("abx", 17);
	tp.print(std::cout);
	std::cout << std::endl << std::endl;

	tp.addkey("abcd", 55);
	tp.print(std::cout);
	std::cout << std::endl << std::endl;

	tp.addkey("a", 99);
	tp.print(std::cout);
	std::cout << std::endl << std::endl;

	int id;

	id = tp.search("abx");
	id = tp.search("a");
	id = tp.search("abcd");
	id = tp.search("abc");
}


void test2()
{
	// Test the specialized unit parse tree

	unit_parse_tree.print(std::cout);

	auto fred = unit_parse_tree.find_unitname("Coulomb");
	auto george = unit_parse_tree.find_unitname("F");
	auto harry = unit_parse_tree.find_unitname("furlong");
	auto sally = unit_parse_tree.find_unitname("Cowlomb");
}

void test3()
{
	// Test the specialized metric prefix parse tree

	mpfx_parse_tree.print(std::cout);

	auto fred = mpfx_parse_tree.find_pfxname("p");
	auto george = mpfx_parse_tree.find_pfxname("u");
	auto harry = mpfx_parse_tree.find_pfxname("Q");
	auto sally = mpfx_parse_tree.find_pfxname("milli");
}

int main()
{
	test1();

	std::cout << std::endl << std::endl;

	test2();

	std::cout << std::endl << std::endl;

	test3();

	return 0;
}