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


	// In the code below, I'm just playing around learning how to use move semantics.
	// So what's here may not be necessary, may not make sense, and may be wrong.

	class ResultBase {
	public:
		std::string msg;
	public:
		ResultBase() : msg() { }
		ResultBase(const std::string_view &msg_) : msg(msg_) { }

		ResultBase(const ResultBase& other)
		: ResultBase(other.msg)
		{
			// Copy constructor
		}

		ResultBase(ResultBase&& other) noexcept
		: msg(std::move(other.msg))
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
			}
		}

		void operator=(ResultBase&& other) noexcept
		{
			// move assignment

			if (this != &other)
			{
				msg = std::move(other.msg);
			}
		}

		virtual void Clear()
		{
			ResetMsg();
		}

		void ResetMsg()
		{
			//std::ostringstream().swap(msg); // swap with a default constructed stringstream
			msg.clear();
		}

		virtual const char* what()
		{
			return msg.c_str();
		}

		virtual void Print(std::ostream& sout) const
		{
			sout << msg << '\n';
		}
	};


	// T must be an enumerated type with a NoError member
	// It must have a companion to_string() function

	template<typename T>
	class ResultPkg : public ResultBase {
	public:
		T code;
	public:

		ResultPkg() : ResultBase(), code(T::NoError) { }

		ResultPkg(const std::string_view &msg_, T code_)
		: ResultBase(msg_)
		, code(code_)
		{
		}

		ResultPkg(const ResultPkg& other)
		: ResultBase(other)
		{
			// Copy constructor
		}

		ResultPkg(ResultPkg&& other) noexcept
		: ResultBase(std::move(other))
		, code(other.code)
		{
			// Move constructor bookkeeping
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
			sout << to_string(code) << "[ " << msg << " ]\n";
		}
	};


	// T must be an enumerated type with a NoError member
	// It must have a companion to_string() function

	template<typename T>
	class AugResultPkg : public ResultPkg<T> {
	public:

		using base_type = ResultPkg<T>;

		Extent extent;

	public:

		AugResultPkg() 
		: ResultPkg<T>("", T::NoError)
		, extent() 
		{ 
		}

		AugResultPkg(std::string_view msg_, T code_, const Extent& extent_)
		: ResultPkg<T>(msg_, code_)
		, extent(extent_)
		{
		}

		AugResultPkg(const AugResultPkg& other)
		: ResultPkg<T>(other)
		, extent(other.extent)
		{
			// Copy constructor
		}

		AugResultPkg(AugResultPkg&& other) noexcept
		: ResultPkg<T>(std::move(other))
		, extent(other.extent)
		{
			// Move constructor bookkeeping
			other.extent.Clear();
		}

		virtual ~AugResultPkg() { }

		AugResultPkg& operator=(const AugResultPkg& other)
		{
			// Copy assignment

			if (this != &other)
			{
				ResultPkg<T>::operator=(other);
				extent = other.extent;
			}

			return *this;
		}

		AugResultPkg& operator=(AugResultPkg&& other) noexcept
		{
			// Move assignment

			if (this != &other)
			{
				ResultPkg<T>::operator=(std::move(other));
				extent = other.extent;
				other.extent.Clear();
			}

			return *this;
		}

		virtual void Clear()
		{
			ResultPkg<T>::Clear();
			extent.Clear();
		}

#if 1
		// @@ ?? WHY WON'T THIS COMPILE?
		// WHY DOES IT NOT SEE code or msg unless I qualify
		// the names?

		virtual void Print(std::ostream& sout) const
		{
			sout << to_string(base_type::code) << "[ " << base_type::msg << " ]\n";
		}
#endif
	};



} // End of namespace