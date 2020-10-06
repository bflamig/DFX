#pragma once
#include <string>
#include <iostream>

namespace bryx
{

	struct Extent
	{
		int srow; // starting row of token
		int erow; // ending row of token
		int scol; // starting col of token
		int ecol; // one past ending col of token

		// In the below, we default to a one row extent, but a zero sized column extent

		Extent() : srow(1), erow(2), scol(1), ecol(1) { }
		Extent(int x) : srow(1), erow(2), scol(x), ecol(x) { }
		Extent(int srow_, int scol_) : srow(srow_), erow(srow_ + 1), scol(scol_), ecol(scol_) { }
		Extent(int srow_, int scol_, int ecol_) : srow(srow_), erow(srow_ + 1), scol(scol_), ecol(ecol_) { }
		Extent(const Extent& other) : srow(other.srow), erow(other.erow), scol(other.scol), ecol(other.ecol) { }
		void operator=(const Extent& other) { srow = other.srow; erow = other.erow, scol = other.scol; ecol = other.ecol; }
		void operator=(int x) { srow = 1; erow = 2; scol = x; ecol = x; }
		void Clear() { srow = 1; erow = 2, scol = 1; ecol = 1; }
		void Bump(int n = 1) { ecol += n; }
		void CopyStart(const Extent& other) { srow = other.srow; erow = other.erow; }
		void MakeSingleLineExtent(int posn) { srow = 1; erow = 1; scol = 1; ecol = posn + 1; }
	};


	// T must be an enumerated type with a NoError member
	// Must have a companion to_string() function

	template<typename T>
	class ResultPkg {
	public:
		std::string msg;
		T code;
		Extent extent;
	public:

		ResultPkg() : msg(), code(T::NoError), extent() { }

		ResultPkg(std::string msg_, T code_, const Extent& extent_)
		{
			msg = msg_;
			code = code_;
			extent = extent_;
		}

		ResultPkg(const ResultPkg& other)
		: ResultPkg(other.msg, other.code, other.extent)
		{
			// Copy constructor
		}

		ResultPkg(ResultPkg&& other) noexcept
		: ResultPkg(move(other.msg), other.code, other.extent)
		{
			// Move constructor
			other.code = T::NoError;
			other.extent.Clear();
		}

		ResultPkg& operator=(const ResultPkg& other)
		{
			// Copy assignment

			if (this != &other)
			{
				msg = other.msg;
				code = other.code;
				extent = other.extent;
			}

			return *this;
		}

		ResultPkg& operator=(ResultPkg&& other) noexcept
		{
			// Move assignment

			if (this != &other)
			{
				msg = move(other.msg);
				code = other.code;
				extent = other.extent;
			}

			return *this;
		}

		void Clear()
		{
			code = T::NoError;
			ResetMsg();
		}

		void ResetMsg()
		{
			//std::ostringstream().swap(msg); // swap with a default constructed stringstream
			msg.clear();
		}

		void Print(std::ostream& sout) const
		{
			sout << to_string(code) << " --> " << msg << '\n';
		}
	};

} // End of namespace