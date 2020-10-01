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

// "Bryx" stands for "Bryan Exchange". It's my own file format. It has the same
// structure as Json, in fact, there is a one to one mapping by design. But Bryx
// simplifies some of the syntax. In most places, you don't need quotes in field
// names and keys unless you have white space in them, or some other non-alphanumeric
// characters, or you start the field with a digit when a number is not expected.
// (This is to keep one-to-one design mapping with Json.)
// We also use '=' instead of ':' for property - value separators. It's 
// is simply easier to see than ':'.
// Because the mapping is one-to-one, there are routines built-in to go back and
// forth between Json and Bryx. There is one restriction with Bryx files that 
// differ from Json files: Some elements can not span multiple lines.

#include <iostream>
#include <sstream>
#include <istream>
#include <string>
#include "EngrNum.h"

namespace bryx
{
	enum class TokenEnum
	{
		// Low level, single character tokens

		Alpha,
		LeftBrace,
		RightBrace,
		LeftSquareBracket,
		RightSquareBracket,
		NVSeparator,      // determined at file parse time or via SyntaxMode flag. Can be one of ':' or '='
		TrialNVSeparator, // Can be either ':' or '='
		WhiteSpace,
		DoubleQuote,
		Zero,
		Digit,
		OneNine,
		HexDigit,
		Period,
		PlusSign,
		MinusSign,
		Exponent,
		Comma,
		Escape,
		OtherChar,

		// higher level sequences of character tokens

		QuotedChars,     // sequence of characters surrounded by double quotes
		UnquotedChars,   // sequence of characters that are either alphanumeric, or a select set of characters (esp no ws or quotes or =)
		//WholeNumber,     // sequence of characters that are digits only, with optional sign prefix
		//FloatingNumber,  // sequence of characters that has a valid floating point syntax
		//NumberWithUnits, // sequence of characters that has a valid whole number or floating point syntax, with optional alpha unit characters
		Number,            // sequence of characters that comprise whole numbers, floating point numbers, all with optional metric prefix and units
		True,            // the sequence of characters 'true', no quotes
		False,           // the sequence of characters 'false', no quotes
		Null,            // the sequence of characters 'null', no quotes
		Empty,           // a default token, with no data
		SOT,             // signal start of tokens (ie., we haven't read any in yet)
		EOT,             // signal end of tokens
		ERROR            // signal error
	};

	extern std::string to_string(TokenEnum e);

	inline bool IndicatesEnd(TokenEnum t)
	{
		return t == TokenEnum::EOT;
	}

	inline bool IndicatesError(TokenEnum t)
	{
		return t == TokenEnum::ERROR;
	}

	inline bool IndicatesQuit(TokenEnum t)
	{
		return t == TokenEnum::ERROR || t == TokenEnum::EOT;
	}

	inline bool IsNumeric(TokenEnum t)
	{
		return t == TokenEnum::Number;
	}
	// ////////////////////////////////////////////////////////////////////////////////

	struct TokenExtent
	{
		int srow; // starting row of token
		int erow; // ending row of token
		int scol; // starting col of token
		int ecol; // one past ending col of token

		// In the below, we default to a one row extent, but a zero sized column extent

		TokenExtent() : srow(1), erow(2), scol(1), ecol(1) { }
		TokenExtent(int srow_, int scol_) : srow(srow_), erow(srow_+1), scol(scol_), ecol(scol_) { }
		TokenExtent(int srow_, int scol_, int ecol_) : srow(srow_), erow(srow_ + 1), scol(scol_), ecol(ecol_) { }
		TokenExtent(const TokenExtent& other) : srow(other.srow), erow(other.erow), scol(other.scol), ecol(other.ecol) { }
		TokenExtent& operator=(const TokenExtent& other) { srow = other.srow; erow = other.erow, scol = other.scol; ecol = other.ecol; return *this; }
		void Clear() { srow = 1; erow = 2, scol = 1; ecol = 1; }
		void Bump(int n = 1) { ecol += n; }
		void CopyStart(const TokenExtent & other) { srow = other.srow; erow = other.erow; }
		void MakeSingleLineExtent(int posn) { srow = 1; erow = 1; scol = 1; ecol = posn + 1; }
	};

	// ////////////////////////////////////////////////////////////////////////////////


	enum class LexiResult
	{
		NoError,
		UnexpectedChar,
		UnhandledState,
		UnterminatedString,
		UnexpectedDecimalPoint,
		InvalidEscapedChar,
		InvalidStartingToken,
		Unsupported,
		Unspecified,
		UnexpectedEOF
	};

	extern std::string to_string(LexiResult r);

	// //////////////////////////////////////////////////////////////////////////////////

	constexpr const char* MetricPrefixes[] = { "femto", "pico", "nano", "micro", "milli", "", "kilo", "Mega", "Giga", "Tera", "Peta" };
	constexpr char MetricPrefixMonikerChars[]{ 'f', 'p', 'n', 'u', 'm', '\0', 'k', 'M', 'G', 'T', 'P' };
	constexpr int MetricExps[]{ -5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5 };

	constexpr const char* ratioUnits[] = { "dB", "X", "%" };

	class LexiNumberTraits {
	public:

		int decimal_point_locn; // -1 if no decimal point
		int exponent_locn;      // -1 if no exponent
		int ratio_units_locn;   // -1 if none
		int metric_pfx_locn;    // -1 if none
		int generic_units_locn; // -1 if no other kind of units
		int end_locn;
		bool could_be_a_number; // even if quoted, it could be valid number if quotes were removed

	public:

		LexiNumberTraits() { clear(); }

		void clear()
		{
			decimal_point_locn = -1;
			exponent_locn = -1;
			ratio_units_locn = -1;
			metric_pfx_locn = -1;
			generic_units_locn = -1;
			could_be_a_number = false;
		}

		bool HasDecimal() const { return decimal_point_locn != -1; }
		bool HasExponent() const { return exponent_locn != -1; }
		bool IsWholeNumber() const { return !HasDecimal() && !HasExponent(); }
		bool IsFloatingNumber() const { return HasDecimal() || HasExponent(); }
		bool HasUnits() const { return HasRatioUnits() || HasGenericUnits(); }
		bool HasRatioUnits() const { return ratio_units_locn != -1; }
		bool HasMetricPrefix() const { return metric_pfx_locn != -1; }
		bool HasGenericUnits() const { return generic_units_locn != -1; }
	};

	// //////////////////////////////////////////////////////////////////////////////////

	class LexiResultPkg {
	public:
		std::string msg;
		LexiResult code;
		TokenExtent extent;
	public:

		LexiResultPkg();
		LexiResultPkg(std::string msg_, LexiResult code_, const TokenExtent &extent_);
		LexiResultPkg(const LexiResultPkg& other);
		LexiResultPkg(LexiResultPkg&& other) noexcept;

		LexiResultPkg& operator=(const LexiResultPkg& other);
		LexiResultPkg& operator=(LexiResultPkg&& other) noexcept;

		void Clear();
		void ResetMsg();
		void Print(std::ostream& sout) const;
	};

	// ////////////////////////////////////////////////////////////////////////////////

	class TokenBase {
	public:
		TokenEnum type;
		TokenExtent extent;
		LexiResultPkg result_pkg;        // Will get filled in for error tokens

		static const TokenExtent def_extent;

	public:

		TokenBase(TokenEnum type_, const TokenExtent& extent_);
		explicit TokenBase(TokenEnum type_);
		TokenBase(const TokenBase& other);
		TokenBase(TokenBase&& other) noexcept;

		virtual ~TokenBase() {  }

		TokenBase& operator=(const TokenBase& other);
		TokenBase& operator=(TokenBase&& other) noexcept;

	public:

		void AddResult(const LexiResultPkg& rp);

		//void seek_to(std::streambuf& sb, size_t p) const
		//{
		//	sb.pubseekoff(p, std::ios_base::beg);
		//}

	public:

		bool IsEndToken() const
		{
			return bryx::IndicatesEnd(type);
		}

		bool IsErrorToken() const
		{
			return bryx::IndicatesError(type);
		}

		bool IsQuitToken() const
		{
			return bryx::IndicatesQuit(type);
		}

		virtual bool IsWholeNumber() const
		{
			return false;
		}

		virtual bool IsFloatingPoint() const
		{
			return false;
		}

		virtual bool IsNumberWithUnits() const
		{
			return false;
		}


	public:

		virtual const std::string to_string() const = 0;

		void Print(std::ostream& sout) const;
	};

	// Single char tokens

	// Ordinary simple tokens

	class CharToken : public TokenBase {
	protected:
		char text[2]; // Allow null byte too
	public:

		friend class Lexi;

	public:

		CharToken(TokenEnum type_, char text_, const TokenExtent& extent_);
		CharToken(TokenEnum type_, char text_);
		explicit CharToken(TokenEnum type_);
		CharToken(const CharToken& other);
		CharToken(CharToken&& other) noexcept;

		virtual ~CharToken() {  }

		CharToken& operator=(const CharToken& other);
		CharToken& operator=(CharToken&& other) noexcept;

	public:

		virtual const std::string to_string() const;

	};


	class SimpleToken : public TokenBase {
	protected:
		std::string text; // Will get instantiated with text from the source
	public:

		friend class Lexi;

	public:

		SimpleToken(TokenEnum type_, std::string text_, const TokenExtent &extent_);
		SimpleToken(TokenEnum type_, std::string text_);
		explicit SimpleToken(TokenEnum type_);
		SimpleToken(const SimpleToken& other);
		SimpleToken(SimpleToken&& other) noexcept;

		virtual ~SimpleToken() {  }

		SimpleToken& operator=(const SimpleToken& other);
		SimpleToken& operator=(SimpleToken&& other) noexcept;
			
	public:

		virtual const std::string to_string() const;

	};

	// Number tokens

	class NumberToken : public TokenBase {
	protected:

		std::string text; // Will get instantiated with text from the source

	public:

		friend class Lexi;

		EngrNum engr_num;                // The source text will be analyzed and parsed into this
		LexiNumberTraits number_traits;  // Will get filled in for possible number tokens

	public:

		NumberToken(TokenEnum type_, std::string text_, const TokenExtent& extent_);
		NumberToken(TokenEnum type_, std::string text_);
		explicit NumberToken(TokenEnum type_);
		NumberToken(const NumberToken& other);
		NumberToken(NumberToken&& other) noexcept;

		virtual ~NumberToken() {  }

		NumberToken& operator=(const NumberToken& other);
		NumberToken& operator=(NumberToken&& other) noexcept;

	public:

		void ProcessNum(std::ostream& serr);

	public:

		bool IsWholeNumber() const
		{
			return !number_traits.HasDecimal();
		}

		bool IsFloatingPoint() const
		{
			return number_traits.HasDecimal();
		}

		bool IsNumberWithUnits() const
		{
			return number_traits.HasUnits();
		}

		virtual const std::string to_string() const;
	};


	// ///////////////////////////////////////////////////////////////////////////////////////////////
	// Lexi
	// ////////////////////////////////////////////////////////////////////////////////////////////////

	enum class SyntaxModeEnum
	{
		AutoDetect,
		Json,
		Bryx
	};

	using token_ptr = std::shared_ptr<TokenBase>;

	class Lexi {
	public:

		std::istream src;

		std::ostringstream temp_buf;

		LexiResultPkg last_lexical_error;

		token_ptr prev_token;
		token_ptr curr_token;

		TokenExtent lexi_posn;  // NOTE: We are just using scol and srow of this for positioning info

		int token_cnt;

		bool preserve_white_space;

		SyntaxModeEnum syntax_mode;
		bool debug_mode;

	public:

		Lexi();
		explicit Lexi(std::streambuf* sb_);

		virtual ~Lexi() { };

		void SetStreamBuf(std::streambuf* sb_)
		{
			src.rdbuf(sb_);
		}

		void SetSyntaxMode(SyntaxModeEnum mode_)
		{
			syntax_mode = mode_;
		}

	public:

		inline static bool IsAlpha(int c)
		{
			// Not doing unicode right now
			return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) ? true : false;
		}

		inline static bool IsWhiteSpace(int c)
		{
			return (c == ' ' || c == '\r' || c == '\n' || c == '\t') ? true : false;
		}

		inline static bool IsZero(int c)
		{
			return c == '0';
		}

		inline static bool IsOneNine(int c)
		{
			return (c >= '1' && c <= '9');
		}

		inline static bool IsDigit(int c)
		{
			return IsZero(c) || IsOneNine(c);
		}

		inline static bool IsHexDigit(int c)
		{
			return ((c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) ? true : false;
		}

		inline static bool IsBackSlash(int c)
		{
			return c == '\\';
		}

		inline static bool IsExponentMarker(int c)
		{
			return c == 'e' || c == 'E';
		}

		inline static bool IsSign(int c)
		{
			return c == '+' || c == '-';
		}

		bool IsNVSeparator(int c)
		{
			// WARNING: May not work properly if auto_detect_syntax
			// mode is still in effect.

			return syntax_mode == SyntaxModeEnum::Bryx ? c == '=' : c == ':';
		}

		static bool IsTrialNVSeparator(int c)
		{
			return c == '=' || c == ':';
		}

		char NVSeparator()
		{
			// WARNING: May not work properly if auto_detect_syntax
			// mode is still in effect.

			return syntax_mode == SyntaxModeEnum::Bryx ? '=' : ':';
		}

		SyntaxModeEnum NVSeparatorType(int c)
		{
			return c == '=' ? SyntaxModeEnum::Bryx : SyntaxModeEnum::Json;
		}

		bool StringNeedsQuotes(const std::string& s) const;

		bool NeedsQuotes(const token_ptr& tkn) const;

	protected:

		void LogError(LexiResult result_, std::string msg_, const TokenExtent &eztent_);

		inline void ClearTempBuff()
		{
			std::ostringstream().swap(temp_buf); // swap with a default constructed stringstream
		}

		inline void AppendChar(int c)
		{
			//temp_buf.sputc(c);
			temp_buf.put(c);
		}

		int GetFilteredChar();

		inline int NextPeek()
		{
			// Rather than peek at the current char, we advance past
			// it and peek at the next char.
			GetFilteredChar();
			return src.peek();
		}


		inline int AppendAbsorbPeek(int c)
		{
			AppendChar(c);
			return NextPeek();
		}

		TokenExtent WhereAreWe();

		int SkipWhiteSpace();

	public:

		token_ptr Start();

		token_ptr Peek()
		{
			return curr_token;
		}

		token_ptr Next();

		void AcceptToken(token_ptr tkn, bool advance);

	protected:

		LexiResult PerformSanityCheck();
		LexiResult CollectSingleCharToken(TokenEnum type, int c);
		LexiResult CollectQuotedChars();
		int HandleEscapedChar(int c);
		LexiResult CollectUnquotedChars();
		LexiResult CollectNumber();

	public:

		static std::shared_ptr<NumberToken> CollectQuotedNumber(std::ostream &serr, const std::string_view text);

	};

}; // end of namespace