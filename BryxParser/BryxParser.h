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


	class ParserResultPkg {
	public:
		std::string msg;
		ParserResult code;
		int token_id;
	public:
		ParserResultPkg();
		ParserResultPkg(std::string msg_, ParserResult code_, int token_id_);
	public:
		void Clear();
		void ResetMsg();
		void Print(std::ostream& sout) const;
	};

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
		Object,          // Bryx object
		Array            // Bryx array
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

		bool is_object() const
		{
			return type == ValueEnum::Object;
		}

		bool is_array() const
		{
			return type == ValueEnum::Array;
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

	using array_type = std::vector<std::shared_ptr<Value>>;

	struct Array : Value {
	public:
		array_type values;
	public:
		Array();
		virtual ~Array();
	public:
		void Add(std::shared_ptr <Value>& v);
	};

	// NOTE: Even though unordered_map below is typed to store "strings" for values, what are really
	// stored are const strings, or at least that's what you get back.

	using object_map_type = std::unordered_map<std::string, std::shared_ptr<Value>>;

	// WARNING! Have to make this struct or msvc compiler complains when using polymorphic make_unique() construction

	struct Object : Value {
	public:

#if 0
		//using raw_map_type = std::vector<std::shared_ptr<NameValue>>;

		using raw_map_type = std::vector<std::pair<std::string, std::shared_ptr<Value>>>;

		// NOTE: Using const std::string as a key will fail to compile for undordered_maps, due to
		// quirk in the standard C++ library (then change the documentation jerks, or at least
		// mention it somewhere. - I wasted hours and hours on this. It works perfectly fine on
		// ordered maps. In either case, it's not necessary to say it at all.
		// Weird too, that my name value type usings const std::string, and the compiler complains
		// not one bit when I passed said string into a std::pair constructor.

		using ordered_map_type = std::map<std::string, std::shared_ptr<Value>>;
		using ordered_multimap_type = std::multimap<std::string, std::shared_ptr<Value>>;

		// Can't use const string here! std::unordered_map<const std::string, int> mymap;
		using unordered_multimap_type = std::unordered_multimap<std::string, std::shared_ptr<Value>>;
#endif

		object_map_type dict;

		MapTypeEnum map_code;

	public:

		Object(MapTypeEnum map_code_)
		: Value(ValueEnum::Object)
		, map_code(map_code_)
		{
			//std::cout << "Object default ctor called\n";
		}

		virtual ~Object()
		{
			//std::cout << "Object dtor called\n";
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

		//using nv_type = std::pair<std::string, std::shared_ptr<Value>>;

		Lexi lexi;

		ParserResultPkg last_parser_error;

		std::shared_ptr<Value> root;
		object_map_type* root_map; // Only valid if we start out with an object

		//std::string file_type_candidates[2];
		//int file_type_code; // index into array above (?)

		std::string file_moniker;

		int curr_token_index;

		bool dfx_mode;

		bool debug_mode;

	public:

		Parser();

		explicit Parser(std::streambuf* sb_);
		virtual ~Parser();

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

		static const object_map_type* ToObjectMap(const std::shared_ptr<Value>& valPtr);
		static object_map_type* ToObjectMap(std::shared_ptr<Value>& valPtr);

		static std::shared_ptr<Value> PropertyExists(const object_map_type* parent_map_ptr, const std::string prop_name);

		static object_map_type* GetObjectProperty(const object_map_type* parent_map_ptr, const std::string prop_name);

		static array_type* ToArray(std::shared_ptr<Value>& valPtr);
		static array_type* GetArrayProperty(const object_map_type* parent_map_ptr, const std::string prop_name);

		static std::shared_ptr<NameValue> ToNameValue(std::shared_ptr<Value>& valPtr);
		// @@ problematic: static std::optional<NameValue> GetNameValueProperty(const object_map_type* parent_map_ptr, const std::string prop_name);

		static std::shared_ptr<SimpleValue> ToSimpleValue(std::shared_ptr<Value>& valPtr);
		static std::optional<std::string> GetSimpleProperty(const object_map_type* parent_map_ptr, const std::string prop_name);

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

		ParserResult CollectObject(std::shared_ptr<Value>& place_holder);
		ParserResult CollectArray(std::shared_ptr<Value>& place_holder);
		ParserResult CollectValue(std::shared_ptr<Value>& place_holder, bool expect, bool dont_allow_nv_pair = false);
		ParserResult CollectMembers(std::shared_ptr<Value>& head_ptr);
		ParserResult CollectMember(std::shared_ptr<NameValue>& place_holder);

		ParserResult CollectElements(std::shared_ptr<Value>& head_ptr);
		ParserResult CollectElement(std::shared_ptr<Value>& place_holder);

	public:

		void PrintError(std::ostream& sout);

		void PrintWalk(std::ostream& sout, const Value& jv, int indent);

		void PrintWalkPair(std::ostream& sout, const nv_type& pair, int indent);

#if 0
		// @@ THIS IS A HACK FOR vector type Object mapping. Notice not const keyword on std::string
		void PrintWalkPair(std::ostream& sout, const std::pair<std::string, std::shared_ptr<Value>>& pair, int indent); // @@ exp
#endif

		void PrintIndent(std::ostream& sout, int indent);
		void NextLine(std::ostream& sout, int indent);
	};


}; // end of namespace