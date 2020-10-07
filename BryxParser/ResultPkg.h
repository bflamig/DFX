#pragma once
#include <string>
#include <iostream>
#include <utility>

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


	// In the code below, I'm just playing around learning how to use move semantics

	class ResultBase {
	public:
		std::string msg;
		Extent extent;
	public:
		ResultBase() : msg(), extent() { }
		ResultBase(std::string_view msg_, const Extent& extent_) : msg(msg_), extent(extent_) { }

		ResultBase(const ResultBase& other)
		: ResultBase(other.msg, other.extent)
		{
			// Copy constructor
		}

		ResultBase(ResultBase&& other) noexcept
		: msg(std::move(other.msg))
		, extent(other.extent)
		{
			// Move constructor
		}

		virtual ~ResultBase() {};

		void operator=(const ResultBase& other)
		{
			// copy assignment 

			if (this != &other)
			{
				msg = other.msg;
				extent = other.extent;
			}
		}

		void operator=(ResultBase&& other) noexcept
		{
			// move assignment

			if (this != &other)
			{
				msg = std::move(other.msg);
				extent = other.extent;
				other.extent.Clear();
			}
		}

		virtual void Clear()
		{
			ResetMsg();
			extent.Clear();
		}

		void ResetMsg()
		{
			//std::ostringstream().swap(msg); // swap with a default constructed stringstream
			msg.clear();
		}


		virtual void Print(std::ostream& sout) const = 0;
	};


	// T must be an enumerated type with a NoError member
	// It must have a companion to_string() function

	template<typename T>
	class ResultPkg : public ResultBase {
	public:
		T code;
	public:

		ResultPkg() : ResultBase(), code(T::NoError) { }

		ResultPkg(std::string_view msg_, T code_, const Extent& extent_)
		: ResultBase(msg_, extent_)
		{
			code = code_;
		}

		ResultPkg(const ResultPkg& other)
		: ResultPkg(other.msg, other.code, other.extent)
		{
			// Copy constructor
		}

		ResultPkg(ResultPkg&& other) noexcept
		: ResultBase(std::move(other.msg), other.extent)
		{
			// Move constructor
			code = other.code;
			other.code = T::NoError;
		}

		virtual ~ResultPkg() { }

		ResultPkg& operator=(const ResultPkg& other)
		{
			// Copy assignment

			if (this != &other)
			{
				ResultBase::operator=(other);
				code = other.code;
			}

			return *this;
		}

		ResultPkg& operator=(ResultPkg&& other) noexcept
		{
			// Move assignment

			if (this != &other)
			{
				ResultBase::operator=(std::move(other));
				code = other.code;
				other.code = T::NoError;
			}

			return *this;
		}

		virtual void Clear()
		{
			ResultBase::Clear();
			code = T::NoError;
		}

		virtual void Print(std::ostream& sout) const
		{
			sout << to_string(code) << " --> " << msg << '\n';
		}
	};

} // End of namespace