#include "SymTree.h"

namespace bryx
{

	int SymTree::add_internal(char c)
	{
		children.push_back(SymElem(c));
		return static_cast<int>(children.size()) - 1;
	}

	size_t SymTree::add_leaf(char c, int id)
	{
		children.push_back(SymElem(c, id));
		return static_cast<int>(children.size()) - 1;
	}

	int SymTree::find_index(char c_)
	{
		int index = -1;

		auto it = std::find_if(children.begin(), children.end(),
			[c_](const SymElem& x)
			{
				return x.c == c_;
			});

		if (it != children.end())
		{
			index = static_cast<int>(std::distance(children.begin(), it));
		}

		return index;
	}

	int SymTree::insert_internal(char c)
	{
		// ASSUMES there are more characters coming after c.
		// This is very important!

		int index = find_index(c);

		if (index == -1)
		{
			// Not already there
			index = add_internal(c);
			auto new_tree = std::make_shared<SymTree>();
			children.at(index).child = new_tree;
		}
		else
		{
			// Already there, but there may not be a new tree
			// for the next char which we ASSUME is coming.
			// So create that new tree

			auto p = children.at(index).child;
			if (!p)
			{
				p = std::make_shared<SymTree>();
				children.at(index).child = p;
			}
		}

		return index;
	}

	int SymTree::insert_leaf(char c, int id)
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

	void SymTree::addstring(std::string_view s, int id)
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
				tp = p.get();
			}
			else
			{
				auto idx = tp->insert_leaf(c, id);
			}
		}
	}

	int SymTree::search(std::string_view s)
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

	void SymTree::print(std::ostream& sout, int indent)
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
