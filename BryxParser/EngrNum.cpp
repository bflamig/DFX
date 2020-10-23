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
#include <ostream>
#include <cmath>
#include <charconv>
#include <cctype>
#include <memory>
#include "EngrNum.h"
#include "BryxLexi.h"
#include "Units.h"

namespace bryx
{
	// ///////////////////////////////////////////////////////////////////////////////////////////

	// /////////////////////////////////////////////////////////////////////////////

	std::string to_string(EngrNumResult r)
	{
		std::string s;

		switch (r)
		{
			case EngrNumResult::NoError: s = "no error";  break;
			case EngrNumResult::InvalidStartCharacter: s = "invalid start character"; break;
			case EngrNumResult::DecimalPointOrDigitExpected: s = "decimal point or digit expected"; break;
			case EngrNumResult::ExpectingExponentDigits: s = "expecting exponent digits"; break;
			case EngrNumResult::UnexpectedCharacter: s = "unexpected character"; break;
			case EngrNumResult::TooManySpaces: s = "too many spaces";  break;
			case EngrNumResult::TooManyGroupDigits: s = "too many group digits";  break;
			case EngrNumResult::MustHaveThreeGroupDigits: s = "must have three group digits";  break;
			case EngrNumResult::ExpectedGroupDigit: s = "expected group digit";  break;
			case EngrNumResult::NumericTokenExpected: s = "numeric token expected";  break;
			case EngrNumResult::NumericOverflow: s = "numeric overflow"; break;
			case EngrNumResult::IncorrectOrUnrecognizedUnits: s = "incorrect or unrecognizable digits"; break;
			case EngrNumResult::ErrorBuildingMantissa: s = "error building mantissa"; break;
			case EngrNumResult::ErrorExtractingTensExponent: s = "error extracting tens exponent"; break;
			case EngrNumResult::LexicalError: s = "lexical error"; break;
			case EngrNumResult::UnspecifiedError:
			default: s = "unspecified error"; break;
		}

		return s;
	}

	// ///////////////////////////////////////////////////////////////////////////////////////////

	EngrNum::EngrNum()
	: sign(1)
	, engr_exp(0)
	, MEXP_MULT(3)
	, tens_exp(0)
	, error_code(EngrNumResult::NoError)
	, value_flag(EngrNumFlags::Ordinary)
	, units(UnitEnum::None)
	{
		mantissa[0] = 0;
		text_units[0] = 0;
	}

	EngrNum::EngrNum(const EngrNum& s)
	{
		operator=(s);
	}

	EngrNum::~EngrNum()
	{
	}

	void EngrNum::operator=(const EngrNum& s)
	{
		memcpy_s(mantissa, ndigits_reserved + 1, s.mantissa, s.ndigits_reserved + 1);
		memcpy_s(mantissa, 32, s.mantissa, 32);
		sign = s.sign;
		engr_exp = s.engr_exp;
		MEXP_MULT = s.MEXP_MULT;
		tens_exp = s.tens_exp;
		error_code = s.error_code;
		value_flag = s.value_flag;
		units = s.units;
	}

	void EngrNum::clear()
	{
		std::memset(mantissa, 0, ndigits_reserved + 1);
		sign = 1;

		engr_exp = 0;
		tens_exp = 0;
		error_code = EngrNumResult::NoError;
		value_flag = EngrNumFlags::Ordinary;
		// units = UnitEnum::None; @@ TODO: SHOULD WE? UPDATE: NO!
	}

	double EngrNum::RawX() const
	{
		// No scaled units like dB applied

		switch (value_flag)
		{
			case EngrNumFlags::Ordinary:
			{
				char* pEnd;
				double d = std::strtod(mantissa, &pEnd);
				if (sign == -1) d = -d;
				int overall_exp = engr_exp * MEXP_MULT + tens_exp;
				d *= pow(10.0, overall_exp);
				return d;
			}
			break;
			case EngrNumFlags::PositiveInfinity:
			{
				return std::numeric_limits<double>::infinity();
			}
			break;
			case EngrNumFlags::NegativeInfinity:
			{
				return -std::numeric_limits<double>::infinity();
			}
			break;
			case EngrNumFlags::NaN:
			default:
			{
				return std::numeric_limits<double>::quiet_NaN();
			}
			break;
		}
	}

	double EngrNum::X() const
	{
		// Applies scaled units like dB

		auto x = RawX();

		if (IsCat<UnitCatEnum::Ratio>(units))
		{
			x = Convert<UnitCatEnum::Ratio>(x, units, UnitEnum::SimpleRatio);
		}
		
		return x;
	}

	void EngrNum::set_num(std::ostream &serr, double d)
	{
		clear();

		if (d < 0)
		{
			sign = -1;
			d = abs(d);
		}
		else sign = 1;

		double new_d;
		convert_to_engr(d, new_d, engr_exp);

		auto [p, ec] = std::to_chars(mantissa, mantissa + ndigits_reserved, new_d);

		if (ec == std::errc())
		{
			// So you can default construct an enumerated type. Who knew? And apparently,
			// the default code ends up being 0, for no error, I presume.

			// Now, chop off the exponent, since we've stored it elsewhere

			// Add a tens exp for test. @@ RMV ASAP
			//*p++ = 'e';
			//*p++ = '+';
			//*p++ = '4';
			p[0] = 0; // Null terminate!
			extract_tens_exp(serr);
			normalize_rhs(serr);
			adjust_trailing_zeros();
    	}
		else
		{
			LogError(serr, EngrNumResult::ErrorBuildingMantissa, "in set_num()", 0);
			value_flag = EngrNumFlags::NaN;
		}
	}

	void EngrNum::set_num(std::ostream& serr, double d, UnitEnum units_)
	{
		units = units_;
		set_num(serr, d);
	}

	void EngrNum::set_num(std::ostream& serr, const NumberToken& tkn)
	{
		clear();

		if (tkn.type == TokenEnum::Number)
		{
			process_num_from_lexi(serr, tkn.to_string().c_str(), tkn.number_traits);
		}
		else if (tkn.type == TokenEnum::Zero)
		{
			set_num(serr, 0.0);
		}
		else
		{
			LogError(serr, EngrNumResult::NumericTokenExpected, "in set_num_from_token()", 0);
		}
	}

	void EngrNum::convert_to_engr(double pos_d, double& new_d, int& mexp)
	{
		// This is the magic formula.
		// WARNING: Only works if d is not zero.
		// WARNING: d must be positive (for some reason).

		new_d = 0.0;
		mexp = 0;

		if (pos_d != 0.0)
		{
			double mlog = log10(pos_d) / MEXP_MULT;
			mexp = int(floor(mlog));
			double f = pow(10.0, MEXP_MULT * mexp);
			new_d = pos_d / f;
		}
	}

	void EngrNum::extract_tens_exp(std::ostream &serr)
	{
		// ASSUMES the mantissa holds only valid chars, not some random outside stuff.
		// ONLY CALL after calling convert_to_engr (that is, if the metric exponent
		// has already been normalized)

		tens_exp = 0;

		char* p = mantissa;
		while (*p != 0)
		{
			if (*p == 'e' || *p == 'E')
			{
				auto starting_p = p++;
				// Must skip past '+' if it's there so to_chars works
				if (*p == '+') ++p;
				// Presume everything else is the exponent
				auto [endp, ec] = std::from_chars(p, mantissa + ndigits_reserved, tens_exp);

				if (ec == std::errc())
				{
					// So you can default construct an enumerated type. Who knew? And apparently,
					// the default code ends up being 0, (meaning no error), I presume.

					// Now, chop off the exponent, since we've stored it elsewhere

					*starting_p = 0;
					error_code = EngrNumResult::NoError;
					value_flag = EngrNumFlags::Ordinary;
				}
				else
				{
					LogError(serr, EngrNumResult::ErrorExtractingTensExponent, "in extract_tens_exp()", 0);
					value_flag = EngrNumFlags::NaN;
				}
				break;
			}
			++p;
		}
	}


	void EngrNum::normalize_rhs(std::ostream &serr)
	{
		// Normalizing the digits to right of decimal point. If there is no
		// decimal point, then we shift left till there is one.
		// TWO PHASES:
		// (1) remove leading zeros by adjusting the exponent
		// (2) if decimal point is more than two digits past the first digit,
		//     (or isn't there at all), then shift decimal point left and
		//     adjust the exponent.

		remove_leading_zeros();
		adjust_decimal_pt(serr);
	}

	void EngrNum::remove_leading_zeros()
	{
		// First goal: Remove all leading zeros on the left side of the decimal pt

		char* p = mantissa;
		char* q = p;

		while (*p == '0')
		{
			q = p;
			++p;
		}

		char* new_mantissa = mantissa;

		if (q != p)
		{
			// The only way we reach here is if there was in fact, at least one
			// leading zero. q points to the final leading zero, p points to one
			// past that. So p points to the rest of the mantissa, which might be
			// null. p might be pointing to a '.'.

			new_mantissa = q;

			// @@ TODO: Probably could fold this in somehow into the shifting in the main loop
			//strcpy_s(mantissa, ndigits_reserved + 1, q); // sz includes room for null terminator
		}

		// Second goal: if we now have a new mantissa that starts out
		// with "0." then we need to continue on past the decimal point,
		// getting rid of the rest of the leading zeros. Except, now, we have
		// to adjust the exponent (metric and tens).

		// If first two chars aren't those below, do nothing. It
		// means we aren't in the right form for there to be a rhs
		// of the right form.

		if (new_mantissa[0] == '0' && new_mantissa[1] == '.')
		{
			if (tens_exp > 0)
			{
				tens_exp--;
			}
			else
			{
				tens_exp = 2; // @@ TODO: MEXP_MULT - 1 ?
				--engr_exp;
			}

			char* p = new_mantissa + 2; // just past the decimal point

			int cnt = 0;

			while (*p == '0')
			{
				++cnt;

				if (cnt == MEXP_MULT)
				{
					// If there are spare tens exps to absorb, do that,
					// else decrement the metric exponent.
					if (tens_exp > MEXP_MULT)
					{
						tens_exp -= MEXP_MULT;
					}
					else
					{
						--engr_exp;
					}

					cnt = 0;
				}

				++p;
			}

			if (cnt > 0)
			{
				// Handle the remainder of leading zeros. Less than a
				// multiple of 3 digits. So can only adjust tens exp. 
				tens_exp -= cnt;
			}

			if (*p != 0)
			{
				// Not on null termination. We've got some digits to move.
				// Move the first non-zero digit to the very beginning
				new_mantissa[0] = *p++;

				if (*p != 0)
				{
					// Next, move the rest of the digits just to the right of
					// the decimal point.
					char* q = new_mantissa + 1 + 1; // past tens digit and then decimal

					while (*p != 0)
					{
						*q++ = *p++;
					}

					// null terminate. 

					*q = 0;

#if 0				
					In fact, why not just zero out the rest of
					// the mantissa. Makes it cleaner for debugging purposes

					ptrdiff_t d = mantissa - new_mantissa;
					auto len = (new_mantissa + ndigits_reserved - d + 1) - q;  // includes the reserved room for the null terminator

					std::memset(q, 0, len); // ensures null-termination too
#endif
				}

			}
		}
		else
		{
			if (*q == '0')
			{
				// q points to final leading zero, and the next char is *not* a '.'.

				// Now the next character (which p points to) might *be* a null. 
				// In which case, we are basically done. For we have a single digit
				// 0 as the number

				if (*p == 0)
				{
					// Don't do anything
				}
				else
				{
					// Next char is not a null byte, so increment the new mantissa
					// past the '0'
					++new_mantissa;
				}
			}
		}

		// Okay, shift the "new" mantissa to the beginning of mantissa.
		// When we reach the null byte, if we are still in the room of 
		// mantissa, null out all the rest of the bytes as well.

		if (new_mantissa != mantissa)
		{
			p = mantissa;

			int cnt = 0;

			while (*new_mantissa != 0)
			{
				*p++ = *new_mantissa++;
				++cnt;
			}

			// Put in null bytes. Remember that ndigits_reserved does not include
			// room for null byte, but there is room for it in the buffer. So in
			// the code below, we use <= instead of < so that that last byte gets
			// nulled for sure.

			while (cnt <= ndigits_reserved)
			{
				*p++ = 0;
				++cnt;
			}
		}
	}

	void EngrNum::adjust_decimal_pt(std::ostream &serr)
	{
		// If there are more than three digits before the decimal point (or the decimal point
		// isn't there at all, then shift the decimal point left until it is no more than 3
		// digits in, all the while adjusting the exponent accordingly. We wish to keep the
		// tens_exp clear, and only have metric exponents, even if it means the decimal point
		// is one or two digits past the first digit.

		// Find the location of the decimal point

		char* p = mantissa;

		while (*p != 0 && *p != '.')
		{
			++p;
		}

		// If decimal doesn't exist, that's not a problem -- as long as we have less than
		// three digits total. (Why? We don't want superfluous decimal points. But
		// because our main objective is to strive to have no tens exponent, only a
		// metric one. So up to three digits are good. Past three digits, we *will*
		// be ensuring a decimal point.)

		char* decimal_pt_barrier = mantissa + 3;

		if (p > decimal_pt_barrier)
		{
			if (*p == 0) // aka not on an existing decimal point
			{
				// We have more than three digits and no decimal point found. We need
				// to make room for one. If there's room, set the curr null byte to a 
				// decimal point and then, make the *next* char a null byte,
				// thus keeping our null termination. If we couldn't find enough room,
				// we've got an overflow condition.

				char* p_lastnull = mantissa + ndigits_reserved; // points right at last possible null byte // @@ TODO: Is this right?

				if (p >= p_lastnull)
				{
					// @@ OVERFLOW. No room for the decimal point.
					LogError(serr, EngrNumResult::NumericOverflow, "in normalize_rhs(): No room for decimal point", 0);
					value_flag = EngrNumFlags::NaN;
				}
				else // @@ BUG FIX. 01/06/2020 seems we should trap this
				{
					// At this point, p points to the null byte we were on when 
					// we came into this block. Put a decimal point there.
					*p = '.';
					// Okay, ensure a null terminator right afterwards
					*(p + 1) = 0;
				}
			}

			if (error_code == EngrNumResult::NoError)
			{
				// p points to the decimal point. Scan backwards until we get to where the
				// decimal point really should go. And where should that be? No further right
				// than location 4, and might be as low as locn 1. 
				// What's the rationale? Our overarching goal is to not have
				// a non-zero tens exponent. (Ie., we want exponents that are 
				// multiples of three, represented with the metric exponent).
				// So, until that test is met, we swap the decimal point with
				// the character on the left, until our criteria have been met.
				// We keep track how many swaps we've done. That determines what
				// adjustment to the exponent should be. In doing this, we use a 
				// virtual overall exponent, not the metric-tens pair. It's too
				// complicated to increment that pair. Afterwards, we'll set up
				// the metric-tens accordingly.

				char* q = p;   // src: this is where the decimal point is
				--p;           // dest: this is prev char to the left of decimal pt

				int overall_exp = engr_exp * MEXP_MULT + tens_exp;

				int cnt = 0;

				char* soft_barrier_p = mantissa + 3;  // just past the third digit
				char* hard_barrier_p = mantissa + 1;  // absolute final barrier, just past the first digit

				// Go until soft barrier is met. That's our nominal requirement.

				do
				{
					char tmp = *q;
					*q = *p;
					*p = tmp;
					--p; --q;
					++overall_exp;
				} while (q != soft_barrier_p);

				// Now, if the overall exponent is a multiple of three, we are done!
				// If not, keep looping till we are. It shouldn't take more than two
				// more swaps.

				while ((overall_exp % MEXP_MULT) != 0)
				{
					// Not done. So keep swapping to the right till we are. This will
					// take only up to two more swaps.
					char tmp = *q;
					*q = *p;
					*p = tmp;
					--p; --q;
					++overall_exp;
				}

				// Take the overall exponent and go back to metric-tens pair

				engr_exp = overall_exp / MEXP_MULT;
				tens_exp = overall_exp % MEXP_MULT;
			}
		}
		else
		{
			// cases like 1.234 x 10^2 e^-1

			// Now, if the overall exponent is a multiple of three, we are done!
			// If not, scan right, swapping as we go, looping till we are at an
			// multiple of three for the exponent. It shouldn't take more than two
			// more swaps. We have safety check for pointers running out of bounds
			// too.

			int overall_exp = engr_exp * MEXP_MULT + tens_exp;

			char* q = p;
			p++;

			while (*p != 0 && (overall_exp % MEXP_MULT) != 0)  // !What if *p = 0? Is this an error of some kind or what?
			{
				// Not done. So keep swapping to the right till we are. This will
				// take only up to two more swaps.
				char tmp = *q;
				*q = *p;
				*p = tmp;
				++p; ++q;
				--overall_exp;
			}

			// Take the overall exponent and go back to metric-tens pair

			engr_exp = overall_exp / MEXP_MULT;
			tens_exp = overall_exp % MEXP_MULT;

		}
	}

	void EngrNum::adjust_trailing_zeros()
	{
		// adjust metric exponent due to trailing zeros (for sure) if > 3 zeros, adjust mexp, chop off the three zeros, repeat

		char* p = mantissa;

		int n = strlen(p) - 1;

		if (n > 0)
		{
			p += n;

			int dc = 0;
			while (n > 0 && *p == '0')
			{
				++dc;
				if (dc == 3)
				{
					++engr_exp;
					char* q = p;
					*q++ = 0; // this is the only one necessary
					*q++ = 0; // but we'll keep things tidy
					*q++ = 0; // ""

					dc = 0;
				}
				--p;
				--n;
			}
		}
	}

#if 0
	// Not used. Left this in for posterity
	void EngrNum::inc_exp()
	{
		// any other way is madness

		int overall_exp = engr_exp * MEXP_MULT + tens_exp;
		++overall_exp;

		engr_exp = overall_exp / MEXP_MULT;
		tens_exp = overall_exp % MEXP_MULT;
	}

	void EngrNum::dec_exp()
	{
		// any other way is madness

		int overall_exp = engr_exp * MEXP_MULT + tens_exp;
		--overall_exp;

		engr_exp = overall_exp / MEXP_MULT;
		tens_exp = overall_exp % MEXP_MULT;
	}

#endif

	// //////////////////////////////////////////////////////////////////////////////////


	std::ostream& operator<<(std::ostream& sout, const EngrNum& x)
	{
		switch (x.value_flag)
		{
			case EngrNumFlags::NaN:
			{
				sout << "NaN"; // @@ TODO: probably due to error
			}
			break;

			case EngrNumFlags::NegativeInfinity:
			{
				sout << "Negative Infinity";
			}
			break;

			case EngrNumFlags::Ordinary:
			{
				if (x.sign < 0)
				{
					sout << '-';
				}

				sout << x.mantissa;

				if (x.tens_exp != 0)
				{
					if (x.engr_exp != 0) sout << "(";
					sout << "e" << x.tens_exp;
					if (x.engr_exp != 0) sout << ")";
				}

				if (x.engr_exp)
				{
					sout << "x10^" << (x.engr_exp * 3);
				}
			}
			break;

			case EngrNumFlags::PositiveInfinity:
			{
				sout << "Positive Infinity";
			}
			break;
		}

		return sout;
	}

	void EngrNum::process_num_from_lexi(std::ostream& serr, std::string_view src, const LexiNumberTraits& number_traits)
	{
		// Do this first, because reasons (we want x to point to right after number)

		int x = number_traits.units_locn;
		if (x != -1)
		{
			// Parse the units
			std::string_view unit_string = src.substr(x);
			units = unit_parse_tree.find_unitname(unit_string);
			if (units == UnitEnum::None)
			{
				// Because the units location wasn't empty,
				// we know we have some units, we just
				// don't know anything about them.
				// UPDATE: Or we could have garbage characters
				// at the end of the string that maybe we should
				// have errored on?
				units = UnitEnum::Other;
			}
		}
		else x = number_traits.end_locn;

		int y = number_traits.metric_pfx_locn;
		if (y != -1)
		{
			// Parse the metric prefix
			char pfx = src[y];

			auto idx = mpfx_parse_tree.MetricPrefixIndex(pfx);

			if (idx != -1)
			{
				engr_exp = metric_db[idx].metric_exp;
			}
			else engr_exp = 0;
		}
		else y = number_traits.end_locn;

		if (y < x) x = y;

		// At this point, x is offset to just after number

		int n = src.length();
		x = n - x;

		src.remove_suffix(x);

		if (src[0] == '+')
		{
			sign = 1;
			src.remove_prefix(1);
		}
		else if (src[0] == '-')
		{
			sign = -1;
			src.remove_prefix(1);
		}

		n = static_cast<int>(src.length());

		if (x < ndigits_reserved) // Does not include null
		{
			std::memcpy(mantissa, src.data(), n);
			mantissa[n] = 0;

			extract_tens_exp(serr);
			engr_exp += tens_exp / MEXP_MULT;
			tens_exp = tens_exp % MEXP_MULT;
			normalize_rhs(serr);
			adjust_trailing_zeros();
		}
		else
		{
			LogError(serr, EngrNumResult::NumericOverflow, "process_num_from_lexi(): No room for all the incoming digits.", 0);
			value_flag = EngrNumFlags::NaN;
		}
	}

	void EngrNum::parse(std::ostream &serr, const std::string_view &src)
	{
		clear();

		auto tkn_ptr = Lexi::ParseBryxNumber(src);

		auto number_tkn_ptr = std::dynamic_pointer_cast<NumberToken>(tkn_ptr);

		if (number_tkn_ptr)
		{
			*this = number_tkn_ptr->engr_num;
		}
		else
		{
			auto err_tkn_ptr = std::dynamic_pointer_cast<SimpleToken>(tkn_ptr);
			auto& errpkg = err_tkn_ptr->result_pkg;
			// errpkg.code @@ TODO: Where to store this?

			LogError(serr, EngrNumResult::LexicalError, errpkg.msg, errpkg.extent.scol); 
			value_flag = EngrNumFlags::NaN;
		}
	}


	void EngrNum::LogError(std::ostream &serr, EngrNumResult result_, std::string msg_, int posn_) // const TokenExtent& extent_)
	{
		error_code = result_;
		serr << msg_ << std::endl;
		if (posn_ != 0) serr << "near posn: " << posn_ << std::endl;
	}

}; // end of namespace