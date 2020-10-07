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

#include <sstream>
#include "BryxLexi.h"
#include "Units.h"

// /////////////////////////////////////////////////////////////////////////////

namespace bryx
{
	std::string to_string(TokenEnum e)
	{
		std::string s;

		switch (e)
		{
			// Low level

			case TokenEnum::Alpha:				s = "Alpha";		break;
			case TokenEnum::LeftBrace:			s = "LeftBrace";		break;
			case TokenEnum::RightBrace:			s = "RightBrace";		break;
			case TokenEnum::LeftSquareBracket:	s = "LeftSquareBracket";		break;
			case TokenEnum::RightSquareBracket:	s = "RightSquareBracket";		break;
			case TokenEnum::NVSeparator:	    s = "NVSeparator";		break;
			case TokenEnum::TrialNVSeparator:	s = "TrialNVSeparator";		break;
			case TokenEnum::WhiteSpace:		s = "WhiteSpace";		break;
			case TokenEnum::DoubleQuote:	s = "DoubleQuote";		break;
			case TokenEnum::Zero:			s = "Zero";		break;
			case TokenEnum::Digit:			s = "Digit";		break;
			case TokenEnum::OneNine:		s = "OneNine";		break;
			case TokenEnum::HexDigit:		s = "HexDigit";		break;
			case TokenEnum::Period:			s = "Period";		break;
			case TokenEnum::PlusSign:		s = "PlusSign";		break;
			case TokenEnum::MinusSign:		s = "MinusSign";		break;
			case TokenEnum::Exponent:		s = "Exponent";		break;
			case TokenEnum::Comma:			s = "Comma";		break;
			case TokenEnum::Escape:			s = "Escape";		break;
			case TokenEnum::OtherChar:		s = "OtherChar";		break;

				// higher level sequences

			case TokenEnum::QuotedChars:	 s = "QuotedChars";		break;
			case TokenEnum::UnquotedChars:	 s = "UnquotedChars";		break;
			case TokenEnum::Number:			 s = "Number";		break;
			case TokenEnum::True:			 s = "True";		break;
			case TokenEnum::False:			 s = "False";		break;
			case TokenEnum::Null:			 s = "Null";		break;
			case TokenEnum::Empty:           s = "Empty";    break;
			case TokenEnum::SOT:			 s = "SOT";		break;
			case TokenEnum::EOT:			 s = "EOT";		break;
			case TokenEnum::ERROR:	         s = "ERROR";	break;
		}

		return s;
	}

	// /////////////////////////////////////////////////////////////////////////////
	// TokenBase

	TokenBase::TokenBase(TokenEnum type_, const Extent& extent_)
	: type(type_), extent(extent_), result_pkg()
	{
	}

	TokenBase::TokenBase(TokenEnum type_)
	: type(type_), extent(), result_pkg()
	{
	}

	TokenBase::TokenBase(const TokenBase& other)
	: TokenBase(other.type, other.extent)
	{
		// Copy constructor
		result_pkg = other.result_pkg;
	}

	TokenBase::TokenBase(TokenBase&& other) noexcept
	: TokenBase(other.type, other.extent)
	{
		// Move constructor. Note argument is *not* const
		other.extent.Clear();

		//other.result_pkg.msg = "foofoo";

		result_pkg = std::move(other.result_pkg);
	}

	TokenBase& TokenBase::operator=(const TokenBase& other)
	{
		// Copy assignment
		if (this != &other)
		{
			type = other.type;
			extent = other.extent;
			result_pkg = other.result_pkg;
		}

		return *this;
	}

	TokenBase& TokenBase::operator=(TokenBase&& other) noexcept
	{
		// Move assignment. Note argument is *not* const

		if (this != &other)
		{
			type = other.type;
			extent = other.extent;

			other.extent.Clear();

			//other.result_pkg.msg = "foofoo";

			result_pkg = std::move(other.result_pkg);
		}

		return *this;
	}

	void TokenBase::SetResult(const LexiResultPkg& rp)
	{
		result_pkg = rp;
	}


	void TokenBase::Print(std::ostream& sout) const
	{
		if (IsErrorToken())
		{
			result_pkg.Print(sout);
		}
		else
		{
			std::cout << "Token: " << bryx::to_string(type) << ", text = \"" << to_string() << "\"" << '\n';
		}
		std::cout << "on row " << extent.srow << ", near col " << extent.scol << " (note: a tab char counts as one column) \n";
	}

	// /////////////////////////////////////////////////////////////////////////////
	// Char simple tokens

	CharToken::CharToken(TokenEnum type_, char text_, const Extent& extent_)
	: TokenBase(type_, extent_)
	{
		text[0] = text_;
		text[1] = 0;
	}

	CharToken::CharToken(TokenEnum type_, char text_)
	: TokenBase(type_)
	{
		text[0] = text_; 
		text[1] = 0;
	}

	CharToken::CharToken(TokenEnum type_)
	: TokenBase(type_)
	{
		text[0] = 0; 
		text[1] = 0;
	}


	CharToken::CharToken(const CharToken& other)
	: TokenBase(other) // (other.type, other.text, other.extent)
	{
		// Copy constructor
		text[0] = other.text[0];
		text[1] = other.text[1];
	}

	CharToken::CharToken(CharToken&& other) noexcept
	: TokenBase(other)
	{
		// Move constructor. Note argument is *not* const
		text[0] = other.text[0];
		text[1] = other.text[1];
	}

	CharToken& CharToken::operator=(const CharToken& other)
	{
		// Copy assignment
		if (this != &other)
		{
			TokenBase::operator=(other);
			text[0] = other.text[0];
			text[1] = other.text[1];
		}

		return *this;
	}

	CharToken& CharToken::operator=(CharToken&& other) noexcept
	{
		// Move assignment. Note argument is *not* const

		if (this != &other)
		{
			TokenBase::operator=(other);
			text[0] = other.text[0];
			text[1] = other.text[1];
		}

		return *this;
	}

	const std::string CharToken::to_string() const
	{
		return std::string(text);
	}


	// /////////////////////////////////////////////////////////////////////////////
	// Ordinary simple tokens

	SimpleToken::SimpleToken(TokenEnum type_, std::string text_, const Extent &extent_)
	: TokenBase(type_, extent_)
	, text(text_)
	{
	}

	SimpleToken::SimpleToken(TokenEnum type_, std::string text_)
	: TokenBase(type_)
	, text(text_)
	{
	}

	SimpleToken::SimpleToken(TokenEnum type_)
	: TokenBase(type_)
	, text()
	{
	}


	SimpleToken::SimpleToken(const SimpleToken& other)
	: TokenBase(other) // (other.type, other.text, other.extent)
	{
		// Copy constructor
		//result_pkg = other.result_pkg;
	}

	SimpleToken::SimpleToken(SimpleToken&& other) noexcept
	: TokenBase(other)
	//: SimpleToken(other.type, move(other.text), other.extent)
	, text(move(other.text))
	{
		// Move constructor. Note argument is *not* const
		//other.extent.Clear();

		//other.text.clear();

		//other.result_pkg.msg = "foofoo";

		//result_pkg = std::move(other.result_pkg);
	}

	SimpleToken& SimpleToken::operator=(const SimpleToken& other)
	{
		// Copy assignment
		if (this != &other)
		{
			TokenBase::operator=(other);
			//type = other.type;
			//extent = other.extent;
			text = other.text;
			//result_pkg = other.result_pkg;
		}

		return *this;
	}

	SimpleToken& SimpleToken::operator=(SimpleToken&& other) noexcept
	{
		// Move assignment. Note argument is *not* const

		if (this != &other)
		{
			TokenBase::operator=(other);
			//type = other.type;
			//extent = other.extent;

			//other.text = "foofoo";

			// I guess std::strings have move function template
			text = move(other.text); // other.text;

			//other.extent.Clear();
			//other.text.Clear();

			//other.result_pkg.msg = "foofoo";

			//result_pkg = std::move(other.result_pkg);
		}

		return *this;
	}

	const std::string SimpleToken::to_string() const
	{
		return text;
	}


	// /////////////////////////////////////////////////////////////////////////////
	// Number tokens

	NumberToken::NumberToken(TokenEnum type_, std::string text_, const Extent& extent_)
	: TokenBase(type_, extent_)
	, text(text_), number_traits()
	{
	}

	NumberToken::NumberToken(TokenEnum type_, std::string text_)
	: TokenBase(type_)
	, text(text_), number_traits()
	{
	}

	NumberToken::NumberToken(TokenEnum type_)
	: TokenBase(type_)
	, text(), number_traits()
	{
	}


	NumberToken::NumberToken(const NumberToken& other)
	: TokenBase(other) // (other.type, other.text, other.extent)
	{
		// Copy constructor
		number_traits = other.number_traits;
		//result_pkg = other.result_pkg;
	}

	NumberToken::NumberToken(NumberToken&& other) noexcept
	: TokenBase(other)
		//: NumberToken(other.type, move(other.text), other.extent)
		, text(move(other.text))
	{
		// Move constructor. Note argument is *not* const
		//other.extent.Clear();

		//other.text.clear();

		number_traits = other.number_traits;

		other.number_traits.clear();

		//other.result_pkg.msg = "foofoo";

		//result_pkg = std::move(other.result_pkg);
	}

	NumberToken& NumberToken::operator=(const NumberToken& other)
	{
		// Copy assignment
		if (this != &other)
		{
			TokenBase::operator=(other);
			//type = other.type;
			//extent = other.extent;
			text = other.text;
			number_traits = other.number_traits;
			//result_pkg = other.result_pkg;
		}

		return *this;
	}

	NumberToken& NumberToken::operator=(NumberToken&& other) noexcept
	{
		// Move assignment. Note argument is *not* const

		if (this != &other)
		{
			TokenBase::operator=(other);
			//type = other.type;
			//extent = other.extent;
			number_traits = other.number_traits;

			//other.text = "foofoo";

			// I guess std::strings have move function template
			text = move(other.text); // other.text;

			//other.extent.Clear();
			//other.text.Clear();

			other.number_traits.clear();

			//other.result_pkg.msg = "foofoo";

			//result_pkg = std::move(other.result_pkg);
		}

		return *this;
	}

	void NumberToken::ProcessNum(std::ostream &serr)
	{
		engr_num.process_num_from_lexi(serr, text, number_traits);
	}

	const std::string NumberToken::to_string() const
	{
		return text;
	}


	// /////////////////////////////////////////////////////////////////////////////

	std::string to_string(LexiResult r)
	{
		std::string s;

		switch (r)
		{
			case LexiResult::NoError: s = "NoError"; break;
			case LexiResult::UnexpectedChar: s = "UnexpectedChar"; break;
			case LexiResult::UnhandledState: s = "UnhandledState"; break;
			case LexiResult::UnterminatedString: s = "Unterminated string"; break;
			case LexiResult::UnexpectedDecimalPoint: s = "UnexpectedDecimalPoint"; break;
			case LexiResult::InvalidEscapedChar: s = "InvalidEscapedChar"; break;
			case LexiResult::InvalidStartingToken: s = "InvalidStartingToken"; break;
			case LexiResult::Unsupported: s = "Unsupported"; break;
			case LexiResult::Unspecified: s = "Unspecified"; break;
			case LexiResult::UnexpectedEOF: s = "UnexpectedEOF"; break;
			default:; return "unspecified result";  break;
		}

		return s;
	}

	// /////////////////////////////////////////////////////////////////////////////
	//
	// OKAY, the main LEXI class
	//
	// /////////////////////////////////////////////////////////////////////////////

	Lexi::Lexi()
	: src(0)
	, temp_buf()
	, last_lexical_error()
	, prev_token(std::make_shared<CharToken>(TokenEnum::Null))
	, curr_token(std::make_shared<CharToken>(TokenEnum::SOT))
	, lexi_posn()
	, token_cnt(0)
	, preserve_white_space(true)
	, syntax_mode(SyntaxModeEnum::AutoDetect)
	, debug_mode(false)
	{

	}

	Lexi::Lexi(std::streambuf* sb_)
		: Lexi()
	{
		src.rdbuf(sb_);
	}

	bool Lexi::StringNeedsQuotes(const std::string& s) const
	{
		if (s.empty())
		{
			return true;
		}

		if (!isalpha(s[0]))
		{
			return true;
		}

		if (syntax_mode == SyntaxModeEnum::Bryx)
		{
			for (auto ch : s)
			{
				if (!isalnum(ch) && ch != '-' && ch != '_' && ch != '.')
				{
					return true;
				}
			}

			return false;
		}

		return true;
	}

	bool Lexi::NeedsQuotes(const token_ptr &tkn) const
	{
		// WARNING: This function may not work properly if auto-syntax-mode 
		// is still in effect.

		if (tkn->type == TokenEnum::QuotedChars || tkn->type == TokenEnum::UnquotedChars)
		{
			return StringNeedsQuotes(tkn->to_string());
		}
		else if (tkn->IsNumberWithUnits())
		{
			return syntax_mode == SyntaxModeEnum::Json;
		}

		return false;
	}

	void Lexi::LogError(LexiResult result_, std::string msg_, const Extent &extent_)
	{
		auto et = MakeErrorToken(result_, msg_, extent_);
		AcceptToken(et, false); // false: don't absorb character to preserve stream position for debugging purposes
		last_lexical_error = et->result_pkg;
	}

	// A static function for those cases where we need to be independent from a lexi object

	std::shared_ptr<SimpleToken> Lexi::MakeErrorToken(LexiResult result_, std::string msg_, const Extent& extent_)
	{
		LexiResultPkg errpkg(msg_, result_, extent_);
		//extent.ecol = extent.scol + 1; // @@ Why do this?
		auto et = std::make_shared<SimpleToken>(TokenEnum::ERROR, msg_, errpkg.extent);
		et->SetResult(errpkg);
		return et;
	}

	int Lexi::GetFilteredChar()
	{
		int c = src.get();
		if (c == '\r')
		{
		}
		else
		{
			if (c == '\n')
			{
				++lexi_posn.srow; lexi_posn.scol = 1;
			}
			else ++lexi_posn.scol;
		}
		return c;
	}

	Extent Lexi::WhereAreWe()
	{
		// We convert to a zero-sized character extent
		auto posn = lexi_posn;
		posn.erow = posn.srow+1;
		posn.ecol = posn.scol;
		return posn;
	}


	int Lexi::SkipWhiteSpace()
	{
		// If preserve flag set, then we record the whitespace into
		// the temp buffer, otherwise, we don't.

		ClearTempBuff();

		int c = src.peek();

		while (c != EOF)
		{
			if (!isspace(c)) break;

			if (preserve_white_space)
			{
				AppendChar(c);
			}

			c = NextPeek();
		}

		return c;
	}

	token_ptr Lexi::Start()
	{
		token_cnt = 0; lexi_posn.Clear();
		prev_token = std::make_shared<CharToken>(TokenEnum::Null);
		curr_token = std::make_shared<CharToken>(TokenEnum::SOT);
		return curr_token;
	}

	token_ptr Lexi::Next()
	{
		LexiResult result{ LexiResult::NoError };
		last_lexical_error.Clear();

		// Yes, a while loop, but usually just one pass, which is terminated
		// via break or return statement. If there's white space at the beginining,
		// then two loops.

		while (!curr_token->IsQuitToken())
		{
			int c = src.peek();

			if (IsWhiteSpace(c))
			{
				if (preserve_white_space)
				{
					auto start_extent = WhereAreWe();
					c = SkipWhiteSpace();
					auto end_extent = WhereAreWe();
					end_extent.CopyStart(start_extent);
					auto type = TokenEnum::WhiteSpace;
					auto t = std::make_shared<SimpleToken>(type, temp_buf.str(), end_extent); // @@ TODO: NON-SENSICAL for printing
					AcceptToken(t, false); // false = don't absorb current character (it's the one the failed the white space test)
				}
				else
				{
					c = SkipWhiteSpace();
					continue; // Collect next token
				}
			}
			else if (c == '{')
			{
				result = CollectSingleCharToken(TokenEnum::LeftBrace, c);
			}
			else if (c == '[')
			{
				result = CollectSingleCharToken(TokenEnum::LeftSquareBracket, c);
			}
			else if (c == '"')
			{
				GetFilteredChar(); // skip quote
				result = CollectQuotedChars();
			}
			else if (IsAlpha(c))
			{
				result = CollectUnquotedChars();

				if (result == LexiResult::NoError)
				{
					// See if we're one of the special atoms, 'true', 'false', or 'null'
					auto &s = curr_token->to_string();
					if (s == "true") curr_token->type = TokenEnum::True;
					else if (s == "false") curr_token->type = TokenEnum::False;
					else if (s == "null") curr_token->type = TokenEnum::Null;
				}
			}
			else if (IsDigit(c) || c == '-') // NOTE: Bryx numbers can't start with '+'
			{
				result = CollectNumber();
			}
			else if (c == '}')
			{
				result = CollectSingleCharToken(TokenEnum::RightBrace, c);
			}
			else if (c == ']')
			{
				result = CollectSingleCharToken(TokenEnum::RightSquareBracket, c);
			}
			else if (c == ',')
			{
				result = CollectSingleCharToken(TokenEnum::Comma, c);
			}
			else if (IsTrialNVSeparator(c) && syntax_mode == SyntaxModeEnum::AutoDetect)
			{
				// This will kick us out of auto detect mode

				if (c == ':')
				{
					syntax_mode = SyntaxModeEnum::Json;
				}
				else syntax_mode = SyntaxModeEnum::Bryx;

				// And now this will work appropriately
				result = CollectSingleCharToken(TokenEnum::NVSeparator, c);
			}
			else if (IsNVSeparator(c))
			{
				result = CollectSingleCharToken(TokenEnum::NVSeparator, c);
			}
			else if (c == EOF)
			{
				auto extent = WhereAreWe();
				extent.Bump();
				auto ch = unsigned char(c);
				//std::string s; s.push_back(ch);
				auto type = TokenEnum::EOT; // Forces quit
				auto t = std::make_shared<CharToken>(type, ch, extent);
				AcceptToken(t, false); // false: don't absorb character to preserve stream position for debugging purposes
			}
			else
			{
				// Not a valid beginning character of a top-level token

				result = LexiResult::UnexpectedChar;  // Could be more specific
				std::ostringstream msg;
				msg << "Next(): char = '" << (unsigned char)(c) << "'";
				auto extent = WhereAreWe();
				extent.Bump();
				LogError(result, msg.str(), extent);
			}

			break; // Not white space, so we're outta here. Just return the token we got
		}

		return curr_token;
	}


	void Lexi::AcceptToken(token_ptr tkn, bool absorb_char)
	{
		// Careful here. We don't want the pointers crossed
		prev_token = curr_token;
		curr_token = tkn;

		++token_cnt;

		if (absorb_char)
		{
			GetFilteredChar();
		}
	}


	LexiResult Lexi::CollectSingleCharToken(TokenEnum type, int c)
	{
		auto extent = WhereAreWe();
		GetFilteredChar(); // We were peeking at this single character, so absorb it
		extent.Bump();
		auto ch = unsigned char(c);
		//std::string s; s.push_back(ch);
		auto t = std::make_shared<CharToken>(type, ch, extent);
		AcceptToken(t, false);  // true: don't absorb character again
		return LexiResult::NoError;
	}


	LexiResult Lexi::CollectQuotedChars()
	{
		// New fix: Don't let strings span \r or \n's. This
		// will help prevent very large run-away strings.

		LexiResult result{ LexiResult::NoError };
		last_lexical_error.Clear();

		ClearTempBuff();

		// NOTE: In the code below, we ASSUME we stay on one line

		auto start_extent = WhereAreWe();

		Extent bad_extent(-1, -1);

		char expected_char = 0;

		auto extent = start_extent;

		int c = src.peek();

		while (c != EOF)
		{
			if (c == '"')
			{
				c = NextPeek();
				break;     // outta here
			}
			if (IsBackSlash(c))
			{
				// We want to do an escaped character

				c = NextPeek();
				extent.Bump();
				c = HandleEscapedChar(c);
			}

			if (c > 0)
			{
				if (c == '\r' || c == '\n')
				{
					// Terminate the string right here, right now.
					// But since quote didn't come, we have an error.

					bad_extent = extent;

					result = LexiResult::UnterminatedString;
					std::ostringstream msg;
					msg << "CollectQuotedChars(): ending quote for string expected before new line";
					LogError(result, msg.str(), bad_extent);

					break;
				}
				else AppendChar(c);
				c = NextPeek();
				++extent.ecol;
			}
			else
			{
				bad_extent = extent;
				break;
			}
		}

		if (bad_extent.scol >= 0)
		{
			if (c == EOF)
			{
				result = LexiResult::UnexpectedEOF;
				std::ostringstream msg;
				msg << "CollectQuotedChars(): expected character '" << expected_char << '\'';
				LogError(result, msg.str(), bad_extent);
			}
			else
			{
				// other errors were handled in HandleEscapedChar() or in while loop above
				result = last_lexical_error.code; // @@ TODO: ASSERT MUST BE ACTUAL ERROR!
			}
		}
		else
		{
			//auto end_extent = start_extent + extent;
			auto t = std::make_shared<SimpleToken>(TokenEnum::QuotedChars, temp_buf.str(), extent);
			AcceptToken(t, false);  // false: don't advance buffer. We already have.
		}

		return result;
	}


	int Lexi::HandleEscapedChar(int c)
	{
		// @@ Am I supposed to interpret these or not?
		// UPDATE: Let's say we are

		LexiResult result{ LexiResult::NoError };
		last_lexical_error.Clear();

		// NOTE: In the below, we ASSUME we stay on one line

		auto start_extent = WhereAreWe();

		Extent bad_extent(-1, -1);

		auto extent = start_extent;

		if (c == '"')
		{
			return '"';
		}

		if (c == '\\')
		{
			return '\\';
		}

		if (c == '/')
		{
			return '/';
		}

		if (c == 'b')
		{
			return '\b';
		}

		if (c == 'f')
		{
			return '\f';
		}

		if (c == 'r')
		{
			return '\r';
		}

		if (c == 'n')
		{
			return '\n';
		}

		if (c == 't')
		{
			return '\t';
		}

		if (c == 'u')
		{
			// Hex escape: eg: "\uHHHH"

			char hexy[4];

			bool wrong = false;

			for (int i = 0; i < 4; i++)
			{
				c = NextPeek();
				extent.Bump();

				if (IsHexDigit(c) || IsDigit(c))
				{
					hexy[i] = c;
				}
				else
				{
					wrong = true;
					break;
				}
			}

			if (wrong)
			{
				result = LexiResult::InvalidEscapedChar;
				std::ostringstream msg;
				msg << "HandleEscapedChar(" << (unsigned char)(c) << "): hex digit was expected";
				LogError(result, msg.str(), extent); //  bad_extent);
			}
			else
			{
				// Make a char out of it somehow
				//@@ TODO: HANDLE THIS. Right now we error out

				result = LexiResult::Unsupported;
				LogError(result, "HandleEscapedChar(): can't do escaped hex characters yet", extent); // bad_extent);
			}
		}

		if (result == LexiResult::NoError)  // @@ Logic goofy till we fully support the hex stuff
		{
			result = LexiResult::InvalidEscapedChar;
			std::ostringstream msg;
			msg << "HandleEscapedChar(" << (unsigned char)(c) << "): char not allowed to be escaped";
			LogError(result, msg.str(), extent); //  bad_extent);
		}

		return -2; // MEANING bad juju
	}

	LexiResult Lexi::CollectUnquotedChars()
	{
		// Rules: Must start with alpha character.
		// Allows alphanumeric characteres afterwards.
		// Also allows '-', '_', '.'. Does not allow
		// white space, including new lines and what not.

		LexiResult result{ LexiResult::NoError };
		last_lexical_error.Clear();

		ClearTempBuff();

		// NOTE: In the below, we ASSUME we stay on one line

		auto start_extent = WhereAreWe();

		Extent bad_extent(-1, -1);
		char expected_char = 0;

		auto extent = start_extent;

		int c = src.peek();

		if (!isalpha(c) && c != EOF)
		{
			//bad_extent = 0;
			result = LexiResult::UnexpectedChar;
			std::string msg;
			msg = "CollectUnquotedChars(): starting character of unquoted field must be an alpha character";
			LogError(result, msg, extent); // bad_extent);
		}
		else
		{
			while (c != EOF)
			{
				if (isalnum(c) || c == '-' || c == '_' || c == '.')
				{
					AppendChar(c);
					c = NextPeek();
					extent.Bump();
				}
				else if (IsWhiteSpace(c))
				{
					// white space terminates us
					break;
				}
				else if (c == ',')
				{
					// comma terminates us
					break;
				}
				else
				{
					//bad_extent = extent;
					result = LexiResult::UnexpectedChar;
					std::ostringstream msg;
					msg << "CollectUnquotedChars(): an unquoted field cannot contain character '" << char(c) << "'\n";
					LogError(result, msg.str(), extent); // bad_extent);
					break;
				}
			}
		}

		if (bad_extent.scol >= 0)
		{
			if (c == EOF)
			{
				result = LexiResult::UnexpectedEOF;
				std::ostringstream msg;
				msg << "CollectString(): expected character '" << expected_char << '\'';
				LogError(result, msg.str(), bad_extent);
			}
			else
			{
				// other errors were handled in HandleEscapedChar() or in while loop above
				result = last_lexical_error.code; // @@ TODO: ASSERT MUST BE ACTUAL ERROR!
			}
		}
		else
		{
			//auto end_extent = start_extent + extent;
			auto t = std::make_shared<SimpleToken>(TokenEnum::UnquotedChars, temp_buf.str(), extent);
			AcceptToken(t, false);  // false: don't advance buffer. We already have.
		}

		return result;
	}

	LexiResult Lexi::CollectNumber()
	{
		// I'm changing this routine in support of unit suffixes (possibly
		// having metric prefixes.) To parse the latter, we really need a
		// lookahead buffer. What better lookahead buffer than to simply 
		// collect up all characters of the number until the first non
		// number (or unit) character is reached. Then, we'll pass the 
		// collected characters into a string-based parser function.

		// For this to work, we have to ASSUME that if the first character
		// we see passes the test as the start of a number, then the
		// following characters *must* pass the number test, otherwise,
		// our parsing fails for this token, period. Ie., there will be
		// no backtracking outside the number parsing.

		// FURTHER NOTE: We ASSUME THe number stays on one line. A pretty
		// good assumption :)

		LexiResult result{ LexiResult::NoError };
		last_lexical_error.Clear();

		auto start_extent = WhereAreWe();
		auto extent = start_extent;

		Extent bad_extent(-1, -1);

		int c = src.peek();

		// NOTE: The following test has probably already been done, but
		// in case this function is called in isolation:

		if (IsDigit(c) || c == '-')
		{
			// The temp buff is where we collect up the non-whitespace
			// characters of the possible token. How convenient for us.

			ClearTempBuff();

			while (c != EOF)
			{
				if (IsDigit(c) || IsAlpha(c) || c == '-' || c == '+' || c == '.' || c == '%')
				{
					AppendChar(c);
					c = NextPeek();
					extent.Bump();
				}
				else break;
			}

			// Return might be a number token or error token
			auto tknptr = ParseBryxNumber(temp_buf.str());
			AcceptToken(tknptr, false);
		}
		else
		{
			result = LexiResult::UnexpectedChar;
			LogError(result, "Invalid start character for a number", start_extent);
		}

		return result;
	}

	// Support when using string views later on

	inline int bump_char(std::string_view::const_iterator& pit, std::string_view::const_iterator& eit)
	{
		static constexpr char EOS = 0;

		if (pit != eit)
		{
			++pit;
		}

		if (pit == eit)
		{
			return EOS;
		}
		else return *pit;
	}

	inline int peek_char(std::string_view::const_iterator& pit, std::string_view::const_iterator& eit)
	{
		static constexpr char EOS = 0;

		if (pit == eit)
		{
			return EOS;
		}
		else return *pit;
	}

	// A static member function

    std::shared_ptr<TokenBase> Lexi::ParseBryxNumber(std::string_view src)
	{
		// This is a static function.
		// The gist of this copied and pasted from CollectNumber on 12/12/2019. @@ BE SURE TO MAKE THIS TRACK

		// WARNING: The switch to using string_view means no more null termination. So beware!
		// So we've moved on to iterators

		auto s = src.begin();
		auto p = s;
		auto e = src.end();

		static constexpr char EOS = 0;

		LexiResult result{ LexiResult::NoError };

		std::shared_ptr<TokenBase> result_token;

		LexiNumberTraits number_traits;
		number_traits.end_locn = std::distance(s, e); // for default, one past the last character

		bool have_at_least_one_digit = false;

		number_traits.could_be_a_number = false;

		char c = peek_char(p, e);

		// Bryx.org (haha) official grammar says:
		//
		// number
		//    integer fraction exponent units
		// integer
		//    digit
		//    onenine digits
		//    '-' digit
		//    '-' digits
		//    '-' onenine digits
		// fraction
		//     ""
		//     '.' digits
		// exponent
		//	   ""
		//     'E' digits
		//     'E' sign digits
		//     'e' digits
		//     'e' sign digits
		// sign
		//     '+'
		//     '-'
		// digit
		//   '0'
		//   onenine
		// onenine
		//   '1', '2', '3', '4', '5', '6', '7', '8', '9'
		// units
		//   ""
		//   ratio_units
		//   metric_pfx alphas
		//   alphas
		// ratio_units
		//   "dB"
		//   "X"
		//   "%"
		// metric_pfx
		//   'f', 'p', 'n', 'u', 'm', '', 'k', 'M', 'G', 'T', 'P'

		// So, we cannot start with a '+' sign, that's for sure, and not a decimal point.
		// In fact, we must start with either a minus sign, or a number, and multi-digit
		// numbers can't begin with 0. A leading zero can only be used for ... well, a
		// single digit of 0.
		// @@ BUG? 07/20/2020: But what about "0.1234". Isn't that legal json? It would be
		// crazy if it were not. @@ TODO: Need to check that.
		// UPDATE: multi digit numbers can start with 0. Don't know why I didn't think that.
		// However, the grammar as given on card would suggest otherwise.
		// https://github.com/nst/JSONTestSuite/tree/master/test_parsing


		if (c == '-')
		{
			c = bump_char(p, e);
		}

		if (c == '0')
		{
			have_at_least_one_digit = true;
			c = bump_char(p, e);
		}

		// Collect rest of integer portion

		if (isdigit(c))
		{
			have_at_least_one_digit = true;
		}

		if (result == LexiResult::NoError)
		{
			while (isdigit(c))
			{
				c = bump_char(p, e);
			}
		}

		// Collect possible fraction

		if (c == '.')
		{
			number_traits.decimal_point_locn = std::distance(s, p);
			c = bump_char(p, e);

			// Collect up digits

			while (isdigit(c))
			{
				c = bump_char(p, e);
			}
		}

		// Collect up possible signed exponent

		if (c == 'e' || c == 'E')
		{
			number_traits.exponent_locn = std::distance(s, p);
			c = bump_char(p, e);

			// Collect possible sign, either plus or minus

			if (c == '+' || c == '-')
			{
				c = bump_char(p, e);
			}

			// Now collect up digits of the exponent, no white space!

			if (!isdigit(c))
			{
				// Ummm,
				result = LexiResult::UnexpectedChar;
				Extent extent(0, 0, std::distance(s, p));
				result_token = MakeErrorToken(result, "CollectBryxNumber(): expecting first exponent digit of a number", extent);
			}
			else
			{
				while (isdigit(c))
				{
					c = bump_char(p, e);
				}
			}
		}

		// Metric prefixes and/or units are up next.
		// Right now, we have a restricted amount of parsing here.
		// Only single character metrix prefixes for example.
		// Because of this, we can't support meter untis yet, 
		// because m (milli) metric prefix has the same starting
		// character as meter. So down the road we need a more 
		// sophisticated back-tracking parser.

		// Okay, check for any metric prefix. Right now, only 
		// single letter monikers are allowed.
		// NOTE: It just so happens that all units we have do *not*
		// start with any of the characters designated for metric
		// prefixes. We're relying on that, here, actually.
		// @@ Except as noted above, "meter" is a problem.

		auto idx = mpfx_parse_tree.MetricPrefixIndex(c);

		if (idx != -1)
		{
			number_traits.metric_pfx_locn = std::distance(s, p);
			c = bump_char(p, e);
		}

		// Okay, we might have units. Note that all units are alpha only,
		// except for percentages, which can have the short hand of "%".
		// So let's test for that first, and then test for all other units.

		if (c == '%')
		{
			number_traits.units_locn = std::distance(s, p);
			c = bump_char(p, e);
		}
		else if (isalpha(c))
		{
			// On to all other units. We collect up all alpha characters.

			number_traits.units_locn = std::distance(s, p);

			while (isalpha(c))
			{
				c = bump_char(p, e);
			}
		}
		else
		{
			// @@ TODO: ERROR? Unless space?
		}

		if (p != e)
		{
			// We have left over garbage characters. This is now
			// considered a mistake.
			result = LexiResult::UnexpectedChar;
			Extent extent(0, 0, std::distance(s, p));
			result_token = MakeErrorToken(result, "CollectBryxNumber(): unexpected characters", extent);
		}
		else
		{

			// Alrighty then, we've got ourselves a number, supposedly

			if (have_at_least_one_digit)
			{
				number_traits.could_be_a_number = true;

				number_traits.end_locn = std::distance(s, p);

				Extent extent(0, 0, number_traits.end_locn);

				auto t = std::make_shared<NumberToken>(TokenEnum::Number, src.data(), extent);

				t->number_traits = number_traits;
				std::stringstream serr;

				t->ProcessNum(serr);

				auto str = serr.str();

				if (str.empty())
				{
					return t;  // Got's us a real live token!
				}
				else
				{
					result = LexiResult::Unspecified;
					Extent extent(0, 0, std::distance(s, p));
					result_token = MakeErrorToken(result, "CollectBryxNumber(): processing number and/or units", extent);
				}
			}
			else
			{
				result = LexiResult::UnexpectedChar; // @@ TODO put in a better code
				Extent extent(0, 0, std::distance(s, p));
				result_token = MakeErrorToken(result, "CollectBryxNumber(): don't have at least one digit", extent);
			}
		}

		return result_token;
	}
}