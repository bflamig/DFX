#include <iostream>
#include <vector>
#include <string_view>
#include "SymTree.h"

using namespace bryx;

void test1()
{
	auto tp = std::make_shared<SymTree>();

	tp->addstring("abc", 42);
	tp->print(std::cout);
	std::cout << std::endl << std::endl;

	tp->addstring("abx", 17);
	tp->print(std::cout);
	std::cout << std::endl << std::endl;

	tp->addstring("abcd", 55);
	tp->print(std::cout);
	std::cout << std::endl << std::endl;

	tp->addstring("a", 99);
	tp->print(std::cout);
	std::cout << std::endl << std::endl;

	int id;

	id = tp->search("abx");
	id = tp->search("a");
	id = tp->search("abcd");
	id = tp->search("abc");
}

#include "units.h"

class UnitParseTree : private SymTree {
public:

	virtual ~UnitParseTree() = default;

	void addstring(std::string_view s, UnitEnum unit)
	{
		SymTree::addstring(s, static_cast<int>(unit));
	}

	UnitEnum search(std::string_view s)
	{
		auto index = SymTree::search(s);

		if (index == -1)
		{
			return UnitEnum::None;
		}
		else return static_cast<UnitEnum>(index);
	}

	virtual void print(std::ostream& sout, int indent = 0)
	{
		for (auto& v : children)
		{
			for (int i = 0; i < indent; i++) sout << '.';

			sout << "'" << v.c << "'";

			if (v.id != -1)
			{
				// Is a leaf
				sout << " --> " << v.id;
			}

			sout << '\n';

			if (v.child)
			{
				v.child->print(sout, indent + 3);
			}
		}
	}

};

void test2()
{
	const std::vector<std::pair<std::string_view, UnitEnum>> mytable = {
		{"db", UnitEnum::DB},
		{"dB", UnitEnum::DB},
		{"dbm", UnitEnum::DBm},
		{"dBm", UnitEnum::DBm},
		{"ppm", UnitEnum::PPM},

		{"%", UnitEnum::Percent},
		{"percent", UnitEnum::Percent},

		{"X", UnitEnum::Ratio},
		{"ratio", UnitEnum::Ratio},

		{"deg", UnitEnum::Degree},
		{"degree", UnitEnum::Degree},
		{"degrees", UnitEnum::Degree},

		{"rad", UnitEnum::Radian},
		{"radian", UnitEnum::Radian},
		{"radians", UnitEnum::Radian},

		{"R", UnitEnum::Ohm},
		{"O", UnitEnum::Ohm},
		{"Ohm", UnitEnum::Ohm},
		{"Ohms", UnitEnum::Ohm},

		{"F", UnitEnum::Farad},
		{"Farad", UnitEnum::Farad},
		{"Farads", UnitEnum::Farad},

		{"H", UnitEnum::Henry},
		{"Henry", UnitEnum::Henry},
		{"Henries", UnitEnum::Henry},

		{"V", UnitEnum::Volt},
		{"Volt", UnitEnum::Volt},
		{"Volts", UnitEnum::Volt},
		{"Vpeak", UnitEnum::Vpeak},
		{"VoltsPeak", UnitEnum::Vpeak},
		{"Vpp", UnitEnum::Vpp},
		{"VoltsPP", UnitEnum::Vpp},
		{"Vrms", UnitEnum::Vrms},
		{"VoltsRms", UnitEnum::Vrms},

		{"A", UnitEnum::Amp},
		{"Amp", UnitEnum::Amp},
		{"Amps", UnitEnum::Amp},
		{"Apeak", UnitEnum::Apeak},
		{"AmpsPeak", UnitEnum::Apeak},
		{"App", UnitEnum::App},
		{"AmpsPP", UnitEnum::App},
		{"Arms", UnitEnum::Arms},
		{"AmpsRms", UnitEnum::Arms},

		{"C", UnitEnum::Coulomb},
		{"Coulomb", UnitEnum::Coulomb},
		{"Coulombs", UnitEnum::Coulomb},
		{"Cpeak", UnitEnum::Cpeak},
		{"CoulombsPeak", UnitEnum::Cpeak},
		{"Cpp", UnitEnum::Cpp},
		{"CoulombsPP", UnitEnum::Cpp},
		{"Crms", UnitEnum::Crms},
		{"CoulombsRms", UnitEnum::Arms},

		{"W", UnitEnum::Watt},
		{"Watt", UnitEnum::Watt},
		{"Watts", UnitEnum::Watt},

		{"degK", UnitEnum::DegreeK},
		{"degreesK", UnitEnum::DegreeK},

		{"degC", UnitEnum::DegreeC},
		{"degreesC", UnitEnum::DegreeC},

		{"degF", UnitEnum::DegreeF},
		{"degreesF", UnitEnum::DegreeF},

		{"rps", UnitEnum::RadiansPerSec},
		{"radians/sec", UnitEnum::RadiansPerSec},

		{"Hz", UnitEnum::Hertz},
		{"Hertz", UnitEnum::Hertz}
	};

	auto tp = UnitParseTree();

	for (auto& s : mytable)
	{
		tp.addstring(s.first, s.second);
	}

	tp.print(std::cout);

	auto fred = tp.search("Coulomb");
	auto george = tp.search("F");
	auto harry = tp.search("furlong");
	auto sally = tp.search("Cowlomb");
}

int main()
{
	//test1();
	test2();
	return 0;
}