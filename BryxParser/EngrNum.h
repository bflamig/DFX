#pragma once

/******************************************************************************\
 * DFX - "Drum font exchange format" - source code
 *
 * Copyright (c) 2020 by Bryan Flamig
 *
 * This software helps facilitate the real-time playing of multi-layered drum
 * samples, by implementing a language that specifies a master directory of the
 * the sample wave files for a drum kit: where the samples are, what they are
 * for, and a summary of their properties. The DFX format allows modifications
 * of sample levels to achieve a unified, pleasing mix of sounds. Mechanisms
 * such as velocity layers and round robins are supported for this purpose.
 *
 * This exchange format has a one to one mapping to the widely used Json syntax,
 * simplified to be easier to read and write. It is easy to translate DFX files
 * into Json files that can be processed by any software supporting Json syntax.
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

// EngrNum's are designed to represent numbers using metric exponents. They are 
// designed to maintain round-tripping between file and memory which helps 
// facilitate the storage of such numbers in a way that keeps their values from
// changing inadverdantly.

#include <ostream>
//#include "BryxLexi.h"
#include "Units.h"

namespace bryx
{

	class LexiNumberTraits; // forw decl
	class NumberToken; // forw decl

	// /////////////////////////////////////////////////////////////////////
	// flags representing the status of the stored value

	enum class EngrNumFlags
	{
		Ordinary,           // So, thus, use mantissa and engr_exp
		PositiveInfinity,   // Number too big to represent with a double}
		NegativeInfinity,   // Number too small to represent with a double
		NaN                 // Undefined, due to conversion error, etc. 
	};

	enum class EngrNumResult
	{
		NoError,
		InvalidStartCharacter,
		DecimalPointOrDigitExpected,
		ExpectingExponentDigits,
		UnexpectedCharacter,
		TooManySpaces,
		TooManyGroupDigits,
		MustHaveThreeGroupDigits,
		ExpectedGroupDigit,
		NumericTokenExpected,
		NumericOverflow,
		IncorrectOrUnrecognizedUnits,
		ErrorBuildingMantissa,
		ErrorExtractingTensExponent,
		LexicalError,
		UnspecifiedError
	};

	// //////////////////////////////////////////////////////////////////////////////////

	class EngrNum {
	public:

		// NOTE: Mantissa here can also have a decimal point. So it's not quite what you think
		// a mantissa would be. Here, the mantissa is "the number without the exponent part."

		static constexpr int ndigits_reserved = 31; // How many digits do we allow (including decimal point), but not including sign or null terminator

		char mantissa[ndigits_reserved + 1]; // Stores null but does not store the leading sign. Will have |mantissa} < 1000 when normalized.
		char text_units[32];                 // Text version of the units. Used during parsing. Includes null byte.
		int sign;                  // Sign bit. 1 if positive, -1 if negative
		int engr_exp;              // We store the exponent in powers of a thousand or million (to support metric-style engineering notation)
		int MEXP_MULT;             // The multiplier for the engr_exp. Either 3 for "thousands" or 6 for "millions". The latter is for squared units.
		int tens_exp;              // Additional tens exponent that is sometimes tacked on to mantissa (think prefix tracking)
		EngrNumResult error_code;  // If non-zero, then the number is invalid for reasons indicated in the code.
		EngrNumFlags value_flag;   // Always check this first before using the number. We indicate infinity and NaN here.
		UnitEnum units; 

	public:

		explicit EngrNum(int MEXP_MULT_ = 3);
		EngrNum(const EngrNum& s);
		virtual ~EngrNum();

		void operator=(const EngrNum& s);

	public:

		void clear();

		virtual void set_num(std::ostream& serr, double d);
		virtual void set_num(std::ostream& serr, const NumberToken& tkn);
		virtual void parse(std::ostream &serr, const std::string_view &src);
		virtual void process_num_from_lexi(std::ostream& serr, const char* src, const LexiNumberTraits& number_traits);

		virtual double RawX() const;

	protected:

		void convert_to_engr(double pos_d, double& new_d, int& mexp);

		void extract_tens_exp(std::ostream &serr);
		void normalize_rhs(std::ostream &serr);
		void remove_leading_zeros();
		void adjust_decimal_pt(std::ostream &serr);
		void adjust_trailing_zeros();

		void LogError(std::ostream &serr, EngrNumResult result_, std::string msg_, int posn_);

	private:

		//void inc_exp();
		//void dec_exp();
	};

	std::ostream& operator<<(std::ostream& sout, const EngrNum& x);

}; // end of namespace