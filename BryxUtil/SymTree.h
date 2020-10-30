#pragma once

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


#include <iostream>
#include <vector>
#include <string_view>

// /////////////////////////////////////////////////////
// Symbol tree support:
// Supports mapping from a string key to an int.
// This is for unique keys only. Implements a low
// overhead, multi-way tree. Probably faster than
// the usual map algs for the purposes I need them,
// (mapping unit names into unit codes.) In such a 
// database, each level of the tree will probably not
// have many elements, so a brute-force searching 
// through those elements is likely to be fast. Much
// faster than hashing or walking a binary tree.
 
namespace bryx
{
	class SymTree;

	struct SymElem
	{
		std::shared_ptr<SymTree> child;
		int id; // It's important that an id == -1 means "not at the end of a valid key"
		char c;

		SymElem() : child(nullptr), id(-1), c(0) { }
		explicit SymElem(char c_, int id_ = -1) : child(nullptr), id(id_), c(c_) { }

	};

	class SymTree {
	public:

		std::vector<SymElem> children;

	public:

		virtual ~SymTree() = default;

		virtual std::shared_ptr<SymTree> MakeNewTree()
		{
			return std::make_shared<SymTree>();
		}

	protected:

		int add_internal(char c);
		int add_leaf(char c, int id);

		int find_index(char c_) const;

		int insert_internal(char c);
		int insert_leaf(char c, int id);

	public:

		void addkey(std::string_view s, int id);

	public:

		int search(std::string_view s) const;
		virtual void print(std::ostream& sout, int indent = 0) const;
		virtual void print_leaf(std::ostream& sout, int id) const;
	};

}