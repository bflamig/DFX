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

#include <vector>
#include <map>
#include <unordered_map>
#include <optional>
#include "BryxLexi.h"
#include "MapTypeEnum.h"

namespace bryx
{

	// //////////////////////////////////////////////////////

	enum class ParserResult
	{
		NoError,
		LexicalError,
		EndOfListedSequence,
		UnexpectedToken,
		WrongToken,
		TokenNotAllowed,
		InvalidStartingToken,
		InvalidBryxConfiguration,
		CannotDetermineSyntaxMode,
		FileOpenError,
		Unsupported,
		UnexpectedEOT
	};

	extern std::string to_string(ParserResult result);

	using ParserResultPkg = AugResultPkg<ParserResult>;

	// ///////////////////////////////////////////////////////

	enum class ValueEnum
	{
		QuotedString,    // Bryx string
		UnquotedString,  // Bryx unquoted string
		Number,          // Bryx number: whole, floating point, might have optional units
		True,            // Bryx boolean that's true
		False,           // Bryx boolean that's false
		Null,            // Bryx value that's null
		NameValuePair,   // Bryx name value pair
		CurlyList,       // Bryx object
		SquareList       // Bryx array
	};

	extern std::string to_string(ValueEnum v);

	// ///////////////////////////////////////////////////////////////

	class Value {
	public:

		ValueEnum type;
		bool dirty;     // Is what's stored locally different than source

	public:

		Value();
		explicit Value(ValueEnum type_);
		virtual ~Value();

	public:

		bool is_string() const
		{
			return (type == ValueEnum::QuotedString) || (type == ValueEnum::UnquotedString);  // @@ TODO: Need to think about this
		}

		bool is_number() const
		{
			return type == ValueEnum::Number;
		}

		virtual bool is_whole_number() const
		{
			return false;
		}

		virtual bool is_floating_point() const
		{
			return false;
		}

		virtual bool is_number_with_units() const
		{
			return false;
		}

		bool is_curly_list() const
		{
			return type == ValueEnum::CurlyList;
		}

		bool is_square_list() const
		{
			return type == ValueEnum::SquareList;
		}

		bool is_boolean() const
		{
			return (type == ValueEnum::True) || (type == ValueEnum::False);
		}

		bool is_null() const
		{
			return type == ValueEnum::Null;
		}

		bool is_pair() const
		{
			return type == ValueEnum::NameValuePair;
		}
	};

	// ////////////////////////////////////////////////////////////////////////

	// WARNING! Have to make this struct or msvc compiler complains when using polymorphic make_unique() construction

	struct SimpleValue : Value {
	public:
		token_ptr tkn;
	public:
		explicit SimpleValue(ValueEnum type_, token_ptr tkn_);
		virtual ~SimpleValue();

		virtual bool is_whole_number() const
		{
			return tkn->IsWholeNumber();
		}

		virtual bool is_floating_pt() const
		{
			return tkn->IsFloatingPoint();
		}

		virtual bool is_number_with_units() const
		{
			return tkn->IsNumberWithUnits();
		}

		token_ptr CompatibleWithNumber();
	};

	using nv_type = std::pair<const std::string, std::shared_ptr<Value>>;

	// WARNING! Have to make this struct or msvc compiler complains when using polymorphic make_unique() construction

	struct NameValue : Value {
	public:
		nv_type pair;
	public:
		NameValue(const std::string& name_, std::shared_ptr<Value>& val_);
		virtual ~NameValue();
	};

	// WARNING! Have to make this struct or msvc compiler complains when using polymorphic make_unique() construction

	using square_list_type = std::vector<std::shared_ptr<Value>>;

	struct SquareList : Value {
	public:
		square_list_type values;
	public:
		SquareList();
		virtual ~SquareList();
	public:
		void Add(std::shared_ptr <Value>& v);
	};

	// NOTE: Even though map below is typed to store "strings" for values, what are really
	// stored are const strings, or at least that's what you get back.

	using curly_list_type = std::map<std::string, std::shared_ptr<Value>>;

	// WARNING! Have to make this struct or msvc compiler complains when using polymorphic make_unique() construction

	struct CurlyList : Value {
	public:

		curly_list_type dict;
		MapTypeEnum map_code;

	public:

		CurlyList(MapTypeEnum map_code_)
		: Value(ValueEnum::CurlyList)
		, map_code(map_code_)
		{
			//std::cout << "CurlyList default ctor called\n";
		}

		virtual ~CurlyList()
		{
			//std::cout << "CurlyList dtor called\n";
		}

		void Insert(std::shared_ptr<NameValue>& nvp)
		{
			if (map_code == MapTypeEnum::Raw)
			{
				//values.push_back(move(nvp));
				//values.emplace_back(move(nvp->pair));
			}
			else
			{
				// for map only: malwares[nvp->name] = std::move(nvp->val);
				// emplace works for all other types except vectors
				dict.emplace(move(nvp->pair));
			}
		}
	};




	// ////////////////////////////////////////////////////////////////////////
	//
	// Parser
	//
	// ////////////////////////////////////////////////////////////////////////

	class Parser {
	public:

		Lexi lexi;

		ParserResultPkg last_parser_error;

		std::shared_ptr<Value> root;
		curly_list_type* root_map; // Only valid if we start out with a curly list!

		std::string file_moniker;

		int curr_token_index;

		bool dfx_mode;
		bool debug_mode;

	public:

		Parser();

		explicit Parser(std::streambuf* sb_);
		virtual ~Parser();

		ParserResult LoadFile(std::string_view& fname);

		void SetStreamBuf(std::streambuf* sb_)
		{
			lexi.SetStreamBuf(sb_);
		}

		void SetSyntaxMode(SyntaxModeEnum mode_)
		{
			lexi.SetSyntaxMode(mode_);
		}

		void SetDfxMode(bool dfx_mode_)
		{
			dfx_mode = dfx_mode_;
		}

	public:

		// This stuff is useful when "reading" the loaded parse tree

		static const curly_list_type* AsCurlyList(const std::shared_ptr<Value>& valPtr);
		static curly_list_type* AsCurlyList(std::shared_ptr<Value>& valPtr);

		static std::shared_ptr<Value> GetPropertyValue(const curly_list_type* parent_map_ptr, const std::string prop_name);

		static curly_list_type* GetCurlyListProperty(const curly_list_type* parent_map_ptr, const std::string prop_name);

		static square_list_type* AsSquareList(std::shared_ptr<Value>& valPtr);
		static square_list_type* GetSquareListProperty(const curly_list_type* parent_map_ptr, const std::string prop_name);

		static std::shared_ptr<NameValue> AsNameValue(std::shared_ptr<Value>& valPtr);
		// @@ problematic: static std::optional<NameValue> GetNameValueProperty(const curly_list_type* parent_map_ptr, const std::string prop_name);

		static std::shared_ptr<SimpleValue> AsSimpleValue(std::shared_ptr<Value>& valPtr);
		static std::optional<std::string> GetSimpleProperty(const curly_list_type* parent_map_ptr, const std::string prop_name);

	public:

		void LogError(ParserResult result_, std::string msg_, int token_id_);

		ParserResult AdvanceToken();
		ParserResult GuardedAdvanceToken(bool silent = false);
		ParserResult CheckForToken(TokenEnum tval, bool expect, bool silent = false);
		ParserResult CheckForEitherToken(TokenEnum tval1, TokenEnum tval2, bool expect, bool silent = false);

		bool NotAtEnd()
		{
			return !(lexi.curr_token->IsQuitToken());
		}

		bool AtEnd()
		{
			return AtEnd(lexi.curr_token);
		}

		bool AtEnd(token_ptr tkn)
		{
			return tkn->IsQuitToken();
		}

		bool AtErr()
		{
			return AtErr(lexi.curr_token);
		}

		bool AtErr(token_ptr tkn)
		{
			return tkn->type == TokenEnum::ERROR;
		}

		token_ptr Peek()
		{
			return lexi.curr_token;
		}

	public:

		virtual ParserResult Preparse();
		virtual ParserResult Parse();

		ParserResult CollectCurlyList(std::shared_ptr<Value>& place_holder);
		ParserResult CollectSquareList(std::shared_ptr<Value>& place_holder);
		ParserResult CollectValue(std::shared_ptr<Value>& place_holder, bool expect, bool dont_allow_nv_pair = false);
		ParserResult CollectMembers(std::shared_ptr<Value>& head_ptr);
		ParserResult CollectMember(std::shared_ptr<NameValue>& place_holder);

		ParserResult CollectElements(std::shared_ptr<Value>& head_ptr);
		ParserResult CollectElement(std::shared_ptr<Value>& place_holder);

	public:

		void PrintError(std::ostream& sout, std::string_view &ctx);

		void PrintWalk(std::ostream& sout, const Value& jv, int indent);

		void PrintWalkPair(std::ostream& sout, const nv_type& pair, int indent);

#if 0
		// @@ THIS IS A HACK FOR vector type CurlyList mapping. Notice not const keyword on std::string
		void PrintWalkPair(std::ostream& sout, const std::pair<std::string, std::shared_ptr<Value>>& pair, int indent); // @@ exp
#endif

		void PrintIndent(std::ostream& sout, int indent);
		void NextLine(std::ostream& sout, int indent);
	};


}; // end of namespace