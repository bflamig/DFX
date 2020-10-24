/******************************************************************************\
 * Bryx - "Bryan exchange format" - source code
 *
 * Copyright (c) 2020 by Bryan Flamig
 *
 * This exchange format has a one to one mapping to the widely used Json syntax,
 * simplified to be easier to read and write. It is easy to translate Bryx files
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

#include "SymTree.h"

namespace bryx
{
	int SymTree::add_internal(char c)
	{
		children.push_back(SymElem(c));
		return static_cast<int>(children.size()) - 1;
	}

	int SymTree::add_leaf(char c, int id)
	{
		children.push_back(SymElem(c, id));
		return static_cast<int>(children.size()) - 1;
	}

	int SymTree::find_index(char c_) const
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
			auto new_tree = MakeNewTree();
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
				p = MakeNewTree();
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
			// Already there. Now, its id might be different from the
			// stored id. In particular, the stored id might be -1,
			// which means this element wasn't the "end" of a valid
			// key, but merely a prefix. If id == -1, then it should
			// be the case that child is not a nullptr. @@ ASSERT THIS?

			auto& elem = children.at(index);

			if (elem.id == -1)
			{
				// @@ TODO: ASSERT Child not null?
				elem.id = id;
			}
			else
			{
				// @@ TODO:
				// What to do? Replace the id?
				// For now, that's what we'll do

				elem.id = id;
			}
		}

		return index;
	}

	void SymTree::addkey(std::string_view key, int id)
	{
		auto n = key.length();

		auto tp = this;

		for (size_t i = 0; i < n; i++)
		{
			auto c = key[i];

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

	int SymTree::search(std::string_view candy_key) const
	{
		// candy_key means "candidate key" haha.
		// Returns id if found, else -1

		auto n = candy_key.length();

		auto tp = this;

		int id = -1;

		for (size_t i = 0; i < n; i++)
		{
			auto c = candy_key[i];

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
				// our key, then the corresponding element in the 
				// tree better have a non-negative id, which is our signal that
				// we've reached a valid end of a key. Note that the element may
				// also have a child, which means that we are at a valid key
				// end which is also a prefix of another valid key.
				// If, however, the id *is* negative, it means we're not at a
				// valid key end, so our key is for sure just a
				// prefix of another valid key.
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

				if (!tp)
				{
					// Well, according to the tree, there are no more characters
					// in the stored key, yet clearly we have more characters
					// in our candidate key, so this means there is no match.
					// We've only got a partial match, which is an error for now.
					id = -1; 
					break;
				}
			}
		}

		return id;
	}

	void SymTree::print(std::ostream& sout, int indent) const
	{
		for (auto& v : children)
		{
			for (int i = 0; i < indent; i++) sout << '.';

			sout << "'" << v.c << "'";

			if (v.id != -1)
			{
				// Is a leaf
				sout << " --> ";
				print_leaf(sout, v.id);
			}

			sout << '\n';

			if (v.child)
			{
				v.child->print(sout, indent + 3);
			}
		}
	}

	void SymTree::print_leaf(std::ostream &sout, int id) const
	{
		sout << id;
	}
};
