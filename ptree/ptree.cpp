#include <iostream>
#include <vector>
#include <string_view>

class ctree;

struct celem
{
	std::shared_ptr<ctree> child;
	int id; // It's important that an id = -1 means "not at the end of a valid search string"
	char c;

	celem() : child(nullptr), id(-1), c(0) { }
	explicit celem(char c_, int id_ = -1) : child(nullptr), id(id_), c(c_) { }

};

class ctree {
public:

	std::vector<celem> children;

public:

	int add_internal(char c)
	{
		children.push_back(celem(c));
		return static_cast<int>(children.size()) - 1;
	}

	size_t add_leaf(char c, int id)
	{
		children.push_back(celem(c, id));
		return static_cast<int>(children.size()) - 1;
	}

	int find_index(char c_)
	{
		int index = -1;

		auto it = std::find_if(children.begin(), children.end(),
			[c_](const celem& x)
			{
				return x.c == c_;
			});

		if (it != children.end())
		{
			index = static_cast<int>(std::distance(children.begin(), it));
		}

		return index;
	}

	int insert_internal(char c)
	{
		// ASSUMES there are more characters coming after c.
		// This is very important!

		int index = find_index(c);

		if (index == -1)
		{
			// Not already there
			index = add_internal(c);
			auto new_tree = std::make_shared<ctree>();
			children.at(index).child = new_tree;
		}
		else
		{
			// Already there, but there may not be a new tree
			// for the next char which we ASSUME is coming.
			// So create a new tree for it.
			
			auto p = children.at(index).child;
			if (!p)
			{
				p = std::make_shared<ctree>();
				children.at(index).child = p;
			}
		}

		return index;
	}

	int insert_leaf(char c, int id)
	{
		int index = find_index(c);

		if (index == -1)
		{
			// Not already there
			index = add_leaf(c, id);
		}
		else
		{
			// Already there. Now, it's id might be different from the
			// stored id. In particular, the stored id might be -1,
			// which means this element wasn't the "end" of a valid
			// string, but merely prefix. (If id == -1, then it should
			// be the case that child is not a nullptr.) @@ ASSERT THIS?

			auto& elem = children.at(index);

			if (elem.id == -1)
			{
				// @@ ASSSRT Child not null?
				elem.id = id;
			}
			else
			{
				// @@ What to do? Replace the id?
				// For now, that's what we'll do

				elem.id = id;
			}
		}

		return index;
	}

	void addstring(std::string_view s, int id)
	{
		auto n = s.length();

		auto tp = this;

		for (size_t i = 0; i < n; i++)
		{
			auto c = s[i];

			if (i < n - 1)
			{
				auto idx = tp->insert_internal(c);
				auto p = tp->children.at(idx).child;
				//if (!p)
				//{
				//	p = std::make_shared<ctree>();
				//	ctp->children.at(idx).child = p;
				//}
				tp = p.get();
			}
			else
			{
				auto idx = tp->insert_leaf(c, id);
			}
		}
	}

	int search(std::string_view s)
	{
		// Returns id if found, else -1

		auto n = s.length();

		auto tp = this;

		int id = -1;

		for (size_t i = 0; i < n; i++)
		{
			auto c = s[i];

			int index = tp->find_index(c);

			if (index == -1)
			{
				// Didn't find character, so we've failed
				id = -1;
				break;
			}
			else
			{
				// The character was found. Now, if it's the last character in
				// our search string, then the corresponding element in the 
				// tree better have a non-negative id. Such an id is our signal
				// that we've reached a valid search end. Note that the element
				// may have a child, which means that are valid search end is
				// also just a prefix of another valid string. If, however,
				// the id *is* negative, it means we're not at a valid search
				// end, so our search string is just a prefix of another valid
				// string.
				// The upshot is this: Test for a non-negative id. If we find
				// one, our search is a success. Otherwise, not.
				// 

				auto& elem = tp->children.at(index);

				if (i == n - 1)
				{
					id = elem.id;  // sign of id tells the tale
					break;
				}

				// Alrighty then, we should sally forth with the
				// rest of the characters in the string

				tp = elem.child.get();
			}
		}

		return id;
	}



	virtual void walk(std::ostream& sout, int indent)
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
				v.child->walk(sout, indent + 3);
			}
		}
	}
};


void test1()
{
	auto tp = std::make_shared<ctree>();

	tp->addstring("abc", 42);
	tp->walk(std::cout, 0);
	std::cout << std::endl << std::endl;

	tp->addstring("abx", 17);
	tp->walk(std::cout, 0);
	std::cout << std::endl << std::endl;

	tp->addstring("abcd", 55);
	tp->walk(std::cout, 0);
	std::cout << std::endl << std::endl;

	tp->addstring("a", 99);
	tp->walk(std::cout, 0);
	std::cout << std::endl << std::endl;

	int id;

	id = tp->search("abx");
	id = tp->search("a");
	id = tp->search("abcd");
	id = tp->search("abc");
}

#include "units.h"

void test2()
{
	const std::unordered_map<std::string_view, UnitEnum> mytable = {
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

	auto tp = std::make_shared<ctree>();

	for (auto& s : mytable)
	{
		tp->addstring(s.first, static_cast<int>(s.second));
	}

	tp->walk(std::cout, 0);
}

int main()
{
	//test1();
	test2();
	return 0;
}