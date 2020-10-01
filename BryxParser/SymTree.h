#pragma once
#include <iostream>
#include <vector>
#include <string_view>

// Symbol tree support:
// Supports mapping from strings to int.
// Unique strings only. Low overhead,
// multi-way trees. Probably faster than
// the usual map algs for my purposes,
// where each level of the tree does not
// have many elements.

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

	protected:

		int add_internal(char c);
		size_t add_leaf(char c, int id);
		int find_index(char c_);
		int insert_internal(char c);
		int insert_leaf(char c, int id);

	public:

		void addstring(std::string_view s, int id);
		int search(std::string_view s);
		virtual void print(std::ostream& sout, int indent = 0);
	};

}