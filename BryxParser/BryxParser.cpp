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

#include "BryxParser.h"

namespace bryx
{

	std::string to_string(ParserResult result)
	{
		std::string s;

		switch (result)
		{
			case ParserResult::NoError: s = "NoError"; break;
			case ParserResult::LexicalError: s = "LexicalError"; break;
			case ParserResult::EndOfListedSequence: s = "EndOfListedSequence"; break;
			case ParserResult::UnexpectedToken: s = "UnexpectedToken"; break;
			case ParserResult::WrongToken: s = "WrongToken"; break;
			case ParserResult::TokenNotAllowed: s = "TokenNotAllowed"; break;
			case ParserResult::InvalidStartingToken: s = "InvalidStartingToken"; break;
			case ParserResult::InvalidBryxConfiguration: s = "InvalidStartingToken"; break;
			case ParserResult::CannotDetermineSyntaxMode: s = "CannotDetermineSyntaxMode"; break;
			case ParserResult::FileOpenError: s = "FileOpenError"; break;
			case ParserResult::Unsupported: s = "Unsupported"; break;
			case ParserResult::UnexpectedEOT: s = "Unexpected EOT"; break;
			default: break;
		}

		return s;
	}

	ParserResultPkg::ParserResultPkg()
	: msg(), code(ParserResult::NoError), token_id(0)
	{
	}

	ParserResultPkg::ParserResultPkg(std::string msg_, ParserResult code_, int token_id_) 
	: ParserResultPkg()
	{
		msg = msg_;
		code = code_;
		token_id_ = token_id_;
	}

	void ParserResultPkg::Clear()
	{
		code = ParserResult::NoError;
		ResetMsg();
	}

	void ParserResultPkg::ResetMsg()
	{
		msg.clear();
	}

	void ParserResultPkg::Print(std::ostream& sout) const
	{
		sout << to_string(code) << ": " << msg << std::endl;
		//sout << "at token: " << token_id << std::endl;
	}

	// ///////////////////////////////////////////////////

	std::string to_string(ValueEnum v)
	{
		std::string s;

		switch (v)
		{
			case ValueEnum::QuotedString: "QuotedString"; break;
			case ValueEnum::UnquotedString: "UnquotedString"; break;
			case ValueEnum::Number: "Number"; break;
			case ValueEnum::True: "True"; break;
			case ValueEnum::False: "False"; break;
			case ValueEnum::Null: "Null"; break;
			case ValueEnum::NameValuePair: "NameValuePair"; break;
			case ValueEnum::Object: "Object"; break;
			case ValueEnum::Array: "Array"; break;

			default: break;
		}

		return s;
	}

	// /////////////////////////////////////////////////////////////////////////

	Value::Value()
	: type(ValueEnum::Null), dirty(false)
	{
		std::cout << "Value default ctor called\n";
	}

	Value::Value(ValueEnum type_)
	: type(type_), dirty(false)
	{
		//std::cout << "Value ctor called\n";
	}

	Value::~Value()
	{
		//std::cout << "Value dtor called\n";
	}

	// /////////////////////////////////////////////////////////////////////////

	SimpleValue::SimpleValue(ValueEnum type_, token_ptr tkn_)
	: Value(type_), tkn(tkn_) // @@ TODO: Should we be cloning this? Perhaps, perhaps, perhaps
	{
		//std::cout << "SimpleValue default ctor called\n";
	}

	SimpleValue::~SimpleValue()
	{
		//std::cout << "SimpleValue dtor called\n";
	}

	// /////////////////////////////////////////////////////////////////////////


	NameValue::NameValue(const std::string& name_, std::shared_ptr<Value>& val_)
	: Value(ValueEnum::NameValuePair)
	, pair(name_, move(val_))
	{
		//std::cout << "NameValue default ctor called\n";
	};

	NameValue::~NameValue()
	{
		//std::cout << "NameValue dtor called\n";
	}

	// /////////////////////////////////////////////////////////////////////////

	// WARNING! Have to make this struct or msvc compiler complains when using polymorphic make_unique() construction

	Array::Array()
		: Value(ValueEnum::Array)
	{
		//std::cout << "Array default ctor called\n"; 
	}

	Array::~Array()
	{
		//std::cout << "Array dtor called\n"; 
	}

	void Array::Add(std::shared_ptr <Value>& v)
	{
		values.push_back(move(v));
	}


	// //////////////////////////////////////////////////////////////////////////
	//
	//
	// BryParser
	//
	//
	// //////////////////////////////////////////////////////////////////////////

	Parser::Parser()
	: lexi{}
	, root{}
	, root_map{}
	, file_moniker{}
	, curr_token_index{ -1 }
	, dfx_mode{ true }
	, debug_mode{ false }
	{
		lexi.preserve_white_space = false; // This will save our sanity
	}

	Parser::Parser(std::streambuf* sb_)
	: Parser()
	{
		SetStreamBuf(sb_);
	}

	Parser::~Parser()
	{

	}

	// ///////////////////////////////////////////////////////////////////////////
	// Property-value helpers
	// NOTE: These are static functions.

	object_map_type* Parser::ToObjectMap(std::shared_ptr<Value>& valPtr)
	{
		auto fred = std::dynamic_pointer_cast<Object>(valPtr);

		if (fred)
		{
			auto& george = fred->dict;
			return &george;
		}
		else return nullptr;
	}


	const object_map_type* Parser::ToObjectMap(const std::shared_ptr<Value>& valPtr)
	{
		auto fred = std::dynamic_pointer_cast<Object>(valPtr);
			
		if (fred)
		{
			auto& george = fred->dict;
			return &george;
		}
		else return nullptr;
	}

	object_map_type* Parser::GetObjectProperty(const object_map_type* parent_map_ptr, const std::string prop_name)
	{
		// Really need to wrap these with try-catches, because at() throws
		// an exception if thingy not found.

		try
		{
			auto sh_val_ptr = parent_map_ptr->at(prop_name);
			return ToObjectMap(sh_val_ptr);
		}
		catch (...)
		{
			return nullptr;
		}
	}

	array_type* Parser::ToArray(std::shared_ptr<Value>& valPtr)
	{
		auto fred = std::dynamic_pointer_cast<Array>(valPtr);

		if (fred)
		{
			auto& george = fred->values;
			return &george;
		}
		else return nullptr;
	}


	array_type* Parser::GetArrayProperty(const object_map_type* parent_map_ptr, const std::string prop_name)
	{
		// Really need to wrap these with try-catches, because at() throws
		// an exception if thingy not found.

		try
		{
			auto sh_val_ptr = parent_map_ptr->at(prop_name);
			return ToArray(sh_val_ptr);
		}
		catch (...)
		{
			return nullptr;
		}
	}

	std::shared_ptr<NameValue> Parser::ToNameValue(std::shared_ptr<Value>& valPtr)
	{
		return std::dynamic_pointer_cast<NameValue>(valPtr);
	}


#if 0
	// @@ PROBLEMATIC
	std::optional<NameValue> Parser::GetNameValueProperty(const object_map_type* parent_map_ptr, const std::string prop_name)
	{
		try
		{
			auto shptr = parent_map_ptr->at(prop_name);
			auto qq = ToNameValue(shptr);
			auto svptr = qq.get();
			return *svptr;
		}
		catch (...)
		{
			return {}; //  nullptr;
		}
	}
#endif

	std::shared_ptr<SimpleValue> Parser::ToSimpleValue(std::shared_ptr<Value>& valPtr)
	{
		return std::dynamic_pointer_cast<SimpleValue>(valPtr);
	}

	std::shared_ptr<Value> Parser::PropertyExists(const object_map_type* parent_map_ptr, const std::string prop_name)
	{
		try
		{
			auto shptr = parent_map_ptr->at(prop_name);
			return shptr;
		}
		catch (...)
		{
			return nullptr;
		}
	}

	std::optional<std::string> Parser::GetSimpleProperty(const object_map_type* parent_map_ptr, const std::string prop_name)
	{
		try
		{
			auto shptr = parent_map_ptr->at(prop_name);
			auto qq = std::dynamic_pointer_cast<SimpleValue>(shptr);
			auto svptr = qq.get();
			auto &tkn = svptr->tkn;
			return tkn->to_string();
		}
		catch (...)
		{
			return {}; // empty return, whatever that means
		}
	}

	// ///////////////////////////////////////////////////////////////////////////

	void Parser::LogError(ParserResult result_, std::string msg_, int token_id_)
	{
		last_parser_error.ResetMsg();
		last_parser_error.msg = msg_;
		last_parser_error.code = result_;
		last_parser_error.token_id = token_id_;
	}


	ParserResult Parser::AdvanceToken()
	{
		auto result = ParserResult::NoError;
		auto tkn = lexi.Next();

		if (tkn->IsEndToken())
		{
			// We are at the end, my friend
			//++curr_token_index; // @@ SHOULD WE? No because repeated calls means infinity
		}
		else if (tkn->IsErrorToken())
		{
			result = ParserResult::LexicalError;
		}
		else
		{
			if (debug_mode)
			{
				//std::cout << "At token index: " << curr_token_index << std::endl;
				std::cout << "Encountered token: ";
				lexi.curr_token->Print(std::cout);
			}
			// By defn, this means no lexical error, so ...
			++curr_token_index;
		}

		// if no error, curr_token will have the token we just came upon.
		// that token may be an EOT token, by the way

		return result;
	}

	ParserResult Parser::GuardedAdvanceToken(bool silent)
	{
		// By definition, we *expect* another token

		auto result = AdvanceToken();

		if (result == ParserResult::NoError)
		{
			// No error, but we may be at EOT, so check that

			if (AtEnd())
			{
				result = ParserResult::UnexpectedEOT;

				if (!silent)
				{
					LogError(result, "missing tokens", curr_token_index);
				}
			}
		}

		return result;
	}

	ParserResult Parser::CheckForToken(TokenEnum tval, bool expect, bool silent)
	{
		// Looks for token with type tval.
		// Might get an eof error here, which is recorded if silent is false.

		auto result = ParserResult::NoError;

		auto tkn_ref = Peek();

		if (AtEnd(tkn_ref))
		{
			if (expect)
			{
				if (AtErr(tkn_ref))
				{
					result = ParserResult::LexicalError;
				}
				else result = ParserResult::UnexpectedEOT;

				if (!silent)
				{
					std::ostringstream msg;
					msg << "expecting token " << to_string(tval);
					LogError(result, msg.str(), curr_token_index);
				}
			}
		}
		else if (tkn_ref->type != tval)
		{
			// Okay, now look for token

			result = ParserResult::WrongToken;

			if (!silent)
			{
				std::ostringstream msg;
				msg << "expecting token " << to_string(tval);
				LogError(result, msg.str(), curr_token_index);
			}
		}

		return result;
	}


	ParserResult Parser::CheckForEitherToken(TokenEnum tval1, TokenEnum tval2, bool expect, bool silent)
	{
		// Looks for token with either type tval1 or tval2.
		// Might get an eof error here, which is recorded if silent is false.
		// Does *not* advance the token.

		auto result = ParserResult::NoError;

		//auto tkn_ref = Peek();

		bool expect_tokn = true;
		bool silent_fail = true;

		// Look for first choice

		auto pr = CheckForToken(tval1, expect_tokn, silent_fail);

		if (pr == ParserResult::NoError)
		{
			// Got the first choice, so skip past it ("collect" it)
			result = pr;
			//AdvanceToken();
		}
		else if (pr == ParserResult::WrongToken)
		{
			// Not the first choice, so look for second choice

			auto pr2 = CheckForToken(tval2, expect_tokn, silent_fail);

			if (pr2 == ParserResult::NoError)
			{
				// Found the second choice, so skip past it ("collect" it);
				result = pr2;
				//AdvanceToken();
			}
			else
			{
				// We have neither the first or second choice.
				// At this point, we don't have to worry about checking for
				// EOT or lexical error, or what not. Because that would have
				// been handled when testing for the first choice. So we only
				// need concern ourselves about whether to log wrong token

				if (expect)
				{
					// Well, we really expected either first or second choice,
					// and got neither, so ...

					result = pr2; // so we can return the failure

					if (!silent)
					{
						std::ostringstream msg;
						msg << "wrong token, should be either: " << to_string(tval1) << " or " << to_string(tval2);
						LogError(result, msg.str(), curr_token_index);
					}

				}
			}
		}
		else
		{
			// Have either unexpected EOT, or a lexical error of some kind

			result = pr; // So we can return the failure

			if (!silent)
			{
				std::ostringstream msg;

				msg << "CheckForEitherToken(): ";

				if (pr == ParserResult::UnexpectedEOT)
				{
					msg << "unexpected EOT, missing either: " << to_string(tval1) << " or " << to_string(tval2);
				}
				else if (pr == ParserResult::LexicalError)
				{
					msg << "invalid token";
				}
				else
				{
					msg << "Unknown error"; // Don't know man
				}

				LogError(result, msg.str(), curr_token_index);
			}
		}

		return result;
	}

	// //////////////////////////////////////////////////////////////////////////

	ParserResult Parser::Preparse()
	{
		auto result = ParserResult::NoError;

		root_map = 0;
		curr_token_index = 0;

		lexi.Start();

		result = AdvanceToken(); // Must start the ball rolling

		if (result != ParserResult::NoError) return result;

		// We are expecting one of the following two types of files:

		// (1) file_moniker nv-separator { member list }
		// (2) any other kind of value

		// We start by seeing if we have the name of a name value pair.

		if (lexi.curr_token->type == TokenEnum::QuotedChars || lexi.curr_token->type == TokenEnum::UnquotedChars)
		{
			// So far, so good. Let's advance the token where we might see an nv separator

			AdvanceToken(); // Know that after this, prev-token has the name.

			if (result != ParserResult::NoError) return result;

			// Is the current token an nv-separator? Note that if auto-detect was turned on, it will automatically
			// have set the syntax mode, and the test for NVSeparator() will now function as it should.

			if (lexi.curr_token->type == TokenEnum::NVSeparator)
			{
				// With an nv separator, we have established that we should be a name value pair.
				// The name is stored in the prev_token at this point in time. We'll copy it to the 
				// file_moniker for safe keeping.

				file_moniker = lexi.prev_token->to_string();
				
				// Now, if we are using json syntax as determined by the kind of nv separator, then the
				// name in the file MUST have been quoted.

				if (lexi.syntax_mode == SyntaxModeEnum::Json && lexi.prev_token->type != TokenEnum::QuotedChars)
				{
					// OOPS!
					result = ParserResult::InvalidStartingToken;
				}

				if (result != ParserResult::NoError) return result;

				// We're still sitting on the nv separator. Let's advance past that.
				// Now, in dfx mode we expect to see a left-brace. Otherwise, we don't
				// care.

				result = AdvanceToken();

				if (result != ParserResult::NoError) return result;

				if (dfx_mode && lexi.curr_token->type != TokenEnum::LeftBrace)
				{
					// Well, ain't that the deal. We gots ourselves the wrong token.
					result = ParserResult::WrongToken;
				}
			}
			else
			{
				// No nv separator. If auto-detect was turned on, then we're in a pickle as we have
				// no way of knowing what the syntax_mode should be. 

				if (lexi.syntax_mode == SyntaxModeEnum::AutoDetect)
				{
					result = ParserResult::CannotDetermineSyntaxMode;
				}
			}
		}
		else
		{
			// We didn't have a name to start off with, so we better not be in auto-detect
			// mode, because we have no way to determine what the syntax mode is.

			if (lexi.syntax_mode == SyntaxModeEnum::AutoDetect)
			{
				result = ParserResult::CannotDetermineSyntaxMode;
			}
		}
		return result;
	}

	ParserResult Parser::Parse()
	{
		auto result = ParserResult::NoError;

		result = Preparse();

		if (result != ParserResult::NoError)
		{
			std::ostringstream msg;
			msg << "Preparse(): Invalid file start -- " << to_string(lexi.curr_token->type);
			LogError(result, msg.str(), curr_token_index);
			return result;
		}

		if (NotAtEnd())
		{
			if (dfx_mode && !file_moniker.empty())
			{
				// This implies we have a name-value pair. Collect the value part
				// of that pair which should have an object type.

				if (NotAtEnd())
				{
					result = CollectObject(root);

					// We'll store the object map pointer
					// (aka dictionary) as a convenience.

					if (result == ParserResult::NoError)
					{
						root_map = ToObjectMap(root);

						if (root_map == nullptr)
						{
							result = ParserResult::InvalidBryxConfiguration;
							std::ostringstream msg;
							msg << "Parse(): Invalid file configuration" << to_string(lexi.curr_token->type);
							LogError(result, msg.str(), curr_token_index);
						}
					}
				}
			}
			else
			{
				// If you have no file moniker then the file can be structured as
				// any value (might be an object, an array, or a single value 
				// of type string, number, atom, etc. This is per json.org.

				bool expect = true;
				result = CollectValue(root, expect);
			}

			// We now *must* be at the end of the file as far as non-white space tokens 
			// are concerned. Otherwise, we should report an error. This is to 
			// prevent garbage junk accumulating at the end of the file that
			// then bites us in the you know where later.

			if (!AtEnd())
			{
				if (AtErr())
				{
					result = ParserResult::LexicalError;
				}
				else
				{
					// Record error if there's not one already logged
					if (result == ParserResult::NoError)
					{
						result = ParserResult::UnexpectedToken;
						std::ostringstream msg;
						msg << "CollectObject(): token " << to_string(lexi.curr_token->type);
						LogError(result, msg.str(), curr_token_index);
					}
				}
			}

		}

		return result;
	}

	ParserResult Parser::CollectObject(std::shared_ptr<Value>& place_holder)
	{
		auto result = ParserResult::NoError;

		bool silent = false; // want to record eof event

		if (NotAtEnd())
		{
			auto tkn = Peek();

			if (tkn->type != TokenEnum::LeftBrace)
			{
				result = ParserResult::InvalidStartingToken;
				std::ostringstream msg;
				msg << "CollectObject(): token " << to_string(tkn->type);
				LogError(result, msg.str(), curr_token_index);
			}
			else
			{
				// Descend into the abyss now:

				// (1) Advance past left brace, and record error if we are EOF.

				result = GuardedAdvanceToken(false); // false means not silent, so will record error

				if (result == ParserResult::NoError)
				{
					// Make a pointer to our soon-to-be list (aka object), which may end up empty

					std::shared_ptr<Value> new_list;

					// (2) Descend and instantiate new list. 
					//     Might get instantiated as an empty list if EOF token coming in.

					result = CollectMembers(new_list); // This will magically instantiate list

					// Back from the abyss, so  transfer ownership of list to the place_holder

					place_holder = move(new_list);
				}
				else
				{
					std::ostringstream msg;
					msg << "CollectObject(): token " << to_string(tkn->type);
					LogError(result, msg.str(), curr_token_index);
				}
			}
		}

		return result;
	}

	ParserResult Parser::CollectArray(std::shared_ptr<Value>& place_holder)
	{
		auto result = ParserResult::NoError;

		bool silent = false; // want to record eof event

		if (NotAtEnd())
		{
			auto tkn = Peek();

			if (tkn->type != TokenEnum::LeftSquareBracket)
			{
				result = ParserResult::InvalidStartingToken;
				std::ostringstream msg;
				msg << "CollectArray(): token " << to_string(tkn->type);
				LogError(result, msg.str(), curr_token_index);
			}
			else
			{
				// Descend into abyss now:

				// (1) Advance past left bracket, and record error if we are EOF.

				result = GuardedAdvanceToken(false); // false means not silent, so will record error

				// Make a pointer to our soon-to-be list (aka object), which may end up empty

				//auto new_node = make_unique<BryxCollection>(ValueEnum::List); 

				std::shared_ptr<Value> new_list;

				// (2) Descend and instantiate new list. 
				//     Might get instantiated as an empty list if EOF token index coming in.

				result = CollectElements(new_list); // This will magically instantiate list

				// Back from the abyss, so  transfer ownership of list to the place_holder

				place_holder = move(new_list);
			}
		}

		return result;
	}

	ParserResult Parser::CollectValue(std::shared_ptr<Value>& place_holder, bool expect, bool dont_allow_nv_pair)
	{
		auto result = ParserResult::NoError;

		bool silent = false; // want to record either eof event

		if (NotAtEnd())
		{
			// value
			//   object
			//   array
			//   string
			//   number
			//   true
			//   false
			//   null

			auto tkn = Peek();

			if (tkn->type == TokenEnum::LeftBrace)
			{
				result = CollectObject(place_holder); // skips past automatically
			}
			else if (tkn->type == TokenEnum::LeftSquareBracket)
			{
				result = CollectArray(place_holder); // skips past automatically
			}
			else if (tkn->type == TokenEnum::QuotedChars || tkn->type == TokenEnum::UnquotedChars)
			{
				// @@ BUG FIX. We might just have a quoted characters value, OR, we might be
				// on the name token of a name-value pair. So we must test for an nv separator fby value.
				// UPDATE: One twist to all this that we have to prevent name fby nvsep fby name fby nvsep 
				// like sequences. That is, the value of a property value pair can't be another property
				// value pair -- not without {} wrapper's that is.

				// Okay, we gots us a chars token of some kind, so make copy of this token for posterity. Note
				// that we might be throwing this copy away if we don't have name value pair after all.
				// C'est la vie.

				std::string saved_name = lexi.curr_token->to_string();

				// Advance over the chars token. Note that it becomes "prev_token", which we may be
				// taking advantage of shortly.

				AdvanceToken();

				// Okay, check for nv separator token

				auto prc = CheckForToken(TokenEnum::NVSeparator, true, true); // (true, true) == expect, silent if fail

				if (prc == ParserResult::NoError)
				{
					// Yep, indeed we found an nv separator, so that means we need to collect the upcoming value.
					// Isn't recursion fun?

					// But wait, if the "don't allow another nv sep flag" is turned on, then we have a error.

					if (dont_allow_nv_pair)
					{
						result = ParserResult::TokenNotAllowed;
						std::ostringstream msg;
						msg << "CollectValue(): token " << to_string(tkn->type) << '\n';
						msg << "EG: Can't have name-value pair fby another name-value" << '\n';
						LogError(result, msg.str(), curr_token_index);
					}
					else
					{
						// Okay, we must advance beyond the nv separator

						AdvanceToken();

						std::shared_ptr<Value> vp; // Set up pointer to value, initially pointing nowhere.

						// expect value, advance past it, record if fail
						result = CollectValue(vp, true, true); // (true, true) --> expect value, don't allow another nv_pair

						if (result == ParserResult::NoError)
						{
							// We've got all the data we need for our name-value pair.
							// Make us a smart pointer and move contents into the place holder
							// Note that the name is in the prev_token.
							auto nvp = std::make_unique<NameValue>(saved_name, vp);
							place_holder = move(nvp); // Transfer ownership to place_holder. Ain't C++ fun!
						}
					}
				}
				else
				{
					// Not an nv separator, so we just have a simple quoted character token, rightly or wrongly.
					// We throw away the prc result code we just got. If there was an error, it'll be
					// attended to later.

					// Okay, back to that quoted character token. Where did it go? It got advanced over.
					// But no worries, it has now become "prev_token", so we can find it there.

					auto t = lexi.prev_token;
					auto sp = std::make_unique<SimpleValue>(ValueEnum::QuotedString, t); // @@ NEED TO GENERALIZE THIS FOR UNQOUTED POSSIBILITIES TOO
					place_holder = move(sp); // Transfer ownership to place_holder. Ain't C++ fun!
					// NO! We've already skipped past this!    AdvanceToken(); // Skip past this simple token
				}

			}
			else if (tkn->type == TokenEnum::Number)
			{
				//auto val_type = tkn->type == TokenEnum::FloatingNumber ? ValueEnum::FloatingNumber : ValueEnum::WholeNumber;
				//if (tkn->type == TokenEnum::NumberWithUnits) val_type = ValueEnum::NumberWithUnits;
				auto val_type = ValueEnum::Number;
				auto sp = std::make_unique<SimpleValue>(val_type, tkn);
				place_holder = move(sp); // Transfer ownership to place_holder. Ain't C++ fun!
				AdvanceToken(); // Skip past this simple token
			}
			else if (tkn->type == TokenEnum::True || tkn->type == TokenEnum::False)
			{
				auto val_type = tkn->type == TokenEnum::True ? ValueEnum::True : ValueEnum::False;
				auto sp = std::make_unique<SimpleValue>(val_type, tkn);
				place_holder = move(sp); // Transfer ownership to place_holder. Ain't C++ fun!
				AdvanceToken(); // Skip past this simple token
			}
			else if (tkn->type == TokenEnum::Null)
			{
				auto sp = std::make_unique<SimpleValue>(ValueEnum::Null, tkn);
				place_holder = move(sp); // Transfer ownership to place_holder. Ain't C++ fun!
				AdvanceToken(); // Skip past this simple token
			}
		}

		return result;
	}

	ParserResult Parser::CollectMembers(std::shared_ptr<Value>& head_ptr)
	{
		// Collect a {}-list. By definition, such lists are implemented as maps,
		// and as such, each element MUST be in name-value form. No other type
		// of member is allowed.

		auto result = ParserResult::NoError;

		auto lp = std::make_unique<Object>(MapTypeEnum::Unordered_Map);

		while (NotAtEnd())
		{
			// members
			//    member
			//    member ',' members
			//
			// member
			//    ws string ws nvsep element  // iow: name-value pair with possible whitespace
			//
			// element
			//    ws value ws

			// Ignore ws, (we already have).
			// We may have one or more name-value pairs, so first token seen after opening
			// brace is either a quoted_chars token or the ending brace.
			// NOTE: only name-value pairs allowed in the list, not just any value.

			std::shared_ptr<NameValue> nvp; // Hopeful pointer to member

			result = CollectMember(nvp);  // and advance

			if (result == ParserResult::EndOfListedSequence)
			{
				result = ParserResult::NoError;
				break;
			}
			else if (result != ParserResult::NoError) // @@ BUG FIX 12/2/2019
			{
				break;
			}
			else
			{
				// Add member to list
				lp->Insert(nvp);
			}

			// We expect either a comma or and ending right brace

			bool expect_tokn = true;
			bool silent_fail = false;

			result = CheckForEitherToken(TokenEnum::Comma, TokenEnum::RightBrace, expect_tokn, silent_fail);

			if (result == ParserResult::NoError)
			{
				auto t = Peek();
				if (t->type == TokenEnum::RightBrace)
				{
					AdvanceToken();
					break;
				}
				else
				{
					// We got a comma, so advance beyond it.
					AdvanceToken();
				}
			}
			else
			{
				// Have an error
				break; // We will go ..... wait ... we can't go on
			}

			// Ready for next go 'round
		}

		// We have our list constructed, so transfer ownership.

		head_ptr = move(lp);

		// @@ BUG FIX: Must set this in case this object is the only thing in the file and
		// we have error on the last member of the object.

		// We might be sitting on an error, so ...

		if (AtErr())
		{
			result = ParserResult::LexicalError;
		}

		return result;
	}

	ParserResult Parser::CollectMember(std::shared_ptr<NameValue>& place_holder)
	{
		// If member found, it's instantiated, soon to be owned by place_holder.
		// If not found, place_holder stays untouched. Tough if no ending brace
		// found an error is returned.

		auto result = ParserResult::NoError;

		// member
		//    ws string ws nvsep element     // iow: name-value pair with possible whitespace
		//
		// element
		//    ws value ws

		// Whitespace already skipped in Lexi::Scan().
		// We might have a name value pair, or an ending brace.

		bool silent = false; // want to record either eof event

		if (NotAtEnd())
		{
			// We might have an ending brace now

			auto tkn = Peek();

			if (tkn->type == TokenEnum::RightBrace)
			{
				// Got it, so we're outta here
				AdvanceToken();
				return ParserResult::EndOfListedSequence;
			}

			// Not at end of list, so expecting the name of a name-value pair.
			// In syntax mode, the token can either be quoted chars or unquoted chars.
			// In Josn mode, it has to be quoted chars.

			if (lexi.syntax_mode == SyntaxModeEnum::Bryx)
			{
				result = CheckForEitherToken(TokenEnum::QuotedChars, TokenEnum::UnquotedChars, true, false); // (true, false) == expect, record if fail
			}
			else
			{
				result = CheckForToken(TokenEnum::QuotedChars, true, false); // (true, false) == expect, record if fail
			}

			if (result == ParserResult::NoError)
			{
				// Gots us a name token, so make copy the quoted characters for posterity

				std::string saved_name = lexi.curr_token->to_string();  // @@ TODO: Could use prev_token.text after AdvanceToken(); UPDATE: Didn't work.

				// Advance over the name

				AdvanceToken();

				// Okay, now expecting a name-value separator. Might have whitespace first. (already handled)

				result = CheckForToken(TokenEnum::NVSeparator, true, false); // (true, false) == expect, record if fail

				if (result == ParserResult::NoError)
				{
					// Got the separator, now look for the value

					AdvanceToken(); // past separator

					std::shared_ptr<Value> vp; // Set up pointer to value, initially pointing nowhere.

					// expect value, advance past it, record if fail
					result = CollectValue(vp, true, true);  // (true, true) --> expect value, but don't allow another nv pair

					if (result == ParserResult::NoError)
					{
						// We've got all the data we need for our name-value pair.
						// Make us a smart pointer and move contents into the place holder
						// Note that the name is in the prev_token.
						auto nvp = std::make_unique<NameValue>(saved_name, vp);
						place_holder = move(nvp); // Transfer ownership to place_holder. Ain't C++ fun!
					}
				}
			}
		}

		return result;
	}

	ParserResult Parser::CollectElements(std::shared_ptr<Value>& head_ptr)
	{
		// Collect a []-list

		auto result = ParserResult::NoError;

		auto lp = std::make_unique<Array>();

		while (NotAtEnd())
		{
			// elements
			//    element ',' elements
			//
			// element
			//    ws value ws

			// Ignore ws, (we already have).
			// We may have one or more value, or possibly none. So next token after
			// opening bracket might be an ending bracket.

			std::shared_ptr<Value> ep; // Hopeful pointer to element

			result = CollectElement(ep);  // advances too

			if (result == ParserResult::EndOfListedSequence)
			{
				result = ParserResult::NoError;
				break;
			}
			else if (result != ParserResult::NoError) // @@ BUG FIX 12 / 2 / 2019
			{
				break;
			}
			else
			{
				// Add element to list
				lp->Add(ep);
			}

			// We expect either a comma or right square bracket

			bool expect_tokn = true;
			bool silent_fail = false;

			result = CheckForEitherToken(TokenEnum::Comma, TokenEnum::RightSquareBracket, expect_tokn, silent_fail);

			if (result == ParserResult::NoError)
			{
				auto tkn = Peek();
				if (tkn->type == TokenEnum::RightSquareBracket)
				{
					AdvanceToken();
					break;
				}
				else
				{
					// We got a comma, so advance beyond it.
					AdvanceToken();
				}
			}
			else
			{
				// Have an error
				break; // We will go ..... wait ... we can't go on
			}

			// Ready for next go 'round
		}

		// We have our list constructed, so transfer ownership.

		head_ptr = move(lp);

		// @@ BUG FIX: Must set this in case this array is the only thing in the file and
		// we have error on the last element of the array.

		// We might be sitting on an error, so ...

		if (AtErr())
		{
			result = ParserResult::LexicalError;
		}

		return result;
	}

	ParserResult Parser::CollectElement(std::shared_ptr<Value>& place_holder)
	{
		// If element found, it's instantiated, soon to be owned by place_holder.
		// If not found, place_holder stays untouched. Tough if noending brace
		// found an error is returned.

		auto result = ParserResult::NoError;

		// element
		//    ws value ws

		// Therefore, we can skip white space, and then we might have a value or an ending square bracket.
		// NOTE: We assume we've already skipped whitespace in Lexi::Scan().

		bool silent = false; // want to record either eof event

		if (NotAtEnd())
		{
			// We might have an ending square bracket now

			auto tkn = Peek();

			if (tkn->type == TokenEnum::RightSquareBracket)
			{
				// Got it, so we're outta here
				AdvanceToken();
				return ParserResult::EndOfListedSequence;
			}

			// Not at end of list, so expecting a value. We allow whitespace to come first.

			std::shared_ptr<Value> vp; // Set up pointer to value, initially pointing nowhere.

			// expect value, advance past it, record if fail. Allows whitespace intially.
			result = CollectValue(vp, true);

			if (result == ParserResult::NoError)
			{
				// We got our precious value, move it into our place holder
				place_holder = move(vp);
			}
		}

		return result;
	}

	// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void Parser::PrintError(std::ostream& sout)
	{
		last_parser_error.Print(sout);

		if (last_parser_error.code == ParserResult::LexicalError)
		{
			lexi.curr_token->Print(sout); // will be an error token
		}
		else
		{
			lexi.curr_token->Print(sout);
		}
	}

	// //////////////////////////////////////////////////////////////////////////

	static constexpr int indent_amt = 3;

	void Parser::PrintWalk(std::ostream& sout, const Value& jv, int indent)
	{
		switch (jv.type)
		{
			case ValueEnum::QuotedString:   // Bryx string
			{
				auto& sv = dynamic_cast<const SimpleValue&>(jv);
				auto& txt = sv.tkn->to_string();

				if (lexi.StringNeedsQuotes(txt))
				{
					sout << '"' << txt << '"';
				}
				else
				{
					sout << txt;
				}
			}
			break;
			case ValueEnum::UnquotedString:   // Bryx string
			{
				auto& sv = dynamic_cast<const SimpleValue&>(jv);
				auto& txt = sv.tkn->to_string();
				if (lexi.StringNeedsQuotes(txt)) // @@ Don't techinically need this here
				{
					sout << '"' << txt << '"';
				}
				else
				{
					sout << txt;
				}
			}
			break;
			case ValueEnum::Number:    // Bryx number
			{
				auto& sv = dynamic_cast<const SimpleValue&>(jv);
				auto& txt = sv.tkn->to_string();

				if (lexi.NeedsQuotes(sv.tkn))
				{
					sout << '"' << txt << '"';
				}
				else sout << txt;
			}
			break;
			case ValueEnum::True:           // Bryx boolean that's true
			{
				sout << "true";
			}
			break;
			case ValueEnum::False:          // Bryx boolean that's false
			{
				sout << "false";
			}
			break;
			case ValueEnum::Null:           // Bryx value that's null
			{
				sout << "null";
			}
			break;
			case ValueEnum::NameValuePair:  // Bryx name value pair (deprecated)
			{
				auto& nvpair = dynamic_cast<const NameValue&>(jv);
				auto& n = nvpair.pair.first; // name (string)

				if (lexi.StringNeedsQuotes(n))
				{
					sout << '"' << n << '"';
				}
				else
				{
					sout << n;
				}

				sout << ' ' << lexi.NVSeparator() << ' ';

				const auto& legs = *nvpair.pair.second; // val (unique ptr to Value)

				int indentpp = indent + indent_amt;

				PrintWalk(sout, legs, indentpp);
			}
			break;
			case ValueEnum::Object:       // Bryx object
			{
				auto& jvobj = dynamic_cast<const Object&>(jv);
				auto& dict = jvobj.dict;

				if (dict.size() == 0)
				{
					sout << "{}";
				}
				else
				{
					sout << "{";
					NextLine(sout, indent + indent_amt);


					size_t i = 0;
					size_t last = dict.size() - 1;

					for (const auto& pair : dict)
					{
						PrintWalkPair(sout, pair, indent);

						if (i != last)
						{
							sout << ',';
							NextLine(sout, indent + indent_amt);
						}
						++i;
					}

					NextLine(sout, indent);

					sout << "}";
				}
			}
			break;
			case ValueEnum::Array:          // Bryx array
			{
				auto& jvarr = dynamic_cast<const Array&>(jv);
				const auto& vs = jvarr.values;

				if (vs.size() == 0)
				{
					sout << "[]";
				}
				else
				{
					sout << "[";
					NextLine(sout, indent);

					size_t i = 0;
					size_t last = vs.size() - 1;

					PrintIndent(sout, indent_amt);

					for (const auto& v : vs)
					{
						PrintWalk(sout, *v, indent);
						if (i != last)
						{
							sout << ',';
							NextLine(sout, indent + indent_amt);
						}
						++i;
					}

					NextLine(sout, indent);

					sout << "]";
				}
			}
			break;

			default: break;
		}
	}

	void Parser::PrintIndent(std::ostream& sout, int indent)
	{
		// @@TODO: CHINZY!

		for (int i = 0; i < indent; i++)
		{
			sout << ' ';
		}
	}

	void Parser::NextLine(std::ostream& sout, int indent)
	{
		sout << '\n';

		// @@TODO: CHINZY!

		for (int i = 0; i < indent; i++)
		{
			sout << ' ';
		}
	}

	void Parser::PrintWalkPair(std::ostream& sout, const nv_type& pair, int indent)
	{
		if (lexi.StringNeedsQuotes(pair.first))
		{
			sout << '"' << pair.first << '"';  // the name
		}
		else
		{
			sout << pair.first;  // the name
		}

		sout << ' ' << lexi.NVSeparator() << ' ';

		const auto& valley = pair.second;

		PrintWalk(sout, *valley, indent + indent_amt);
	}

}; // end of namespace