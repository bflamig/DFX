#pragma once
#include <ostream>
#include "BryxLexi.h"

namespace bryx
{
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

	class EngrNumResultPkg {
	public:
		std::string msg;
		EngrNumResult code;
		int posn;
		//TokenExtent extent;
	public:

		EngrNumResultPkg();
		EngrNumResultPkg(std::string msg_, EngrNumResult code_, int posn); //  const TokenExtent& extent_);
		EngrNumResultPkg(const EngrNumResultPkg& other);
		EngrNumResultPkg(EngrNumResultPkg&& other) noexcept;

		EngrNumResultPkg& operator=(const EngrNumResultPkg& other);
		EngrNumResultPkg& operator=(EngrNumResultPkg&& other) noexcept;

		void Clear();
		void ResetMsg();
		void Print(std::ostream& sout) const;
	};


	// //////////////////////////////////////////////////////////////////////////////////


	class EngrNum {
	public:

		char* mantissa;            // Skips the leading sign. Will have mantissa < 1000 when normalized.
		int ndigits_reserved;      // How many digits do we allow (including decimal point apparently), but not including sign or null terminator
		int sign;                  // Sign bit. 1 if positive, -1 if negative
		int engr_exp;              // We store the exponent in powers of a thousand or million (to support metric-style engineering notation)
		const int MEXP_MULT;       // The multiplier for the engr_exp. Either 3 for "thousands" or 6 for "millions". The latter is for squared units.
		int tens_exp;              // Additional tens exponent that is sometimes tacked on to mantissa (think prefix tracking)
		EngrNumResult error_code;  // If non-zero, then the number is invalid for reasons indicated in the code.
		EngrNumFlags value_flag;   // Always check this first before using the number. We indicate infinity and NaN here.

	public:

		explicit EngrNum(int ndigits_reserved_, int MEXP_MULT_);
		virtual ~EngrNum();

	public:

		void clear();

		virtual void set_num(std::ostream& serr, double d);
		virtual void set_num(std::ostream& serr, const Token& tkn);
		virtual void parse(std::ostream &serr, const char* src);

		virtual double RawX() const;

	protected:

		void convert_to_engr(double pos_d, double& new_d, int& mexp);

		void extract_tens_exp(std::ostream &serr);
		void normalize_rhs(std::ostream &serr);
		void remove_leading_zeros();
		void adjust_decimal_pt(std::ostream &serr);
		void adjust_trailing_zeros();

		virtual void process_num_from_lexi(std::ostream& serr, const char* src, const LexiNumberTraits& number_traits);

		void LogError(std::ostream &serr, EngrNumResult result_, std::string msg_, int posn_); // const TokenExtent& extent_)

	private:

		//void inc_exp();
		//void dec_exp();
	};

	std::ostream& operator<<(std::ostream& sout, const EngrNum& x);

}; // end of namespace