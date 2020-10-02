#pragma once
#include <iostream>
#include <vector>
#include <string_view>

// /////////////////////////////////////////////////////
// Symbol tree support:
// Supports mapping from a string key to an int.
// This is for unique strings only. We have Low
// overhead, multi-way trees. Probably faster than
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
		int id; // It's important that an id = -1 means "not at the end of a valid search string"
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
		size_t add_leaf(char c, int id);

		int find_index(char c_) const;

		int insert_internal(char c);
		int insert_leaf(char c, int id);

	public:

		void addstring(std::string_view s, int id);

	public:



		int search(std::string_view s) const;
		virtual void print(std::ostream& sout, int indent = 0) const;
		virtual void print_leaf(std::ostream& sout, int id) const;
	};

}