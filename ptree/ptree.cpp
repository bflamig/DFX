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

	tp.addstring("abc", 42);
	tp.print(std::cout);
	std::cout << std::endl << std::endl;

	tp.addstring("abx", 17);
	tp.print(std::cout);
	std::cout << std::endl << std::endl;

	tp.addstring("abcd", 55);
	tp.print(std::cout);
	std::cout << std::endl << std::endl;

	tp.addstring("a", 99);
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

int main()
{
	test1();

	std::cout << std::endl << std::endl;

	test2();
	return 0;
}