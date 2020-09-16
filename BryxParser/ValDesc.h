#pragma once
#include <string>

namespace RcdUtils
{

	using GenericEnum = int;

	// Used in creating lists of enumerated values and their descriptions

	struct ValDesc
	{
		GenericEnum Val;
		std::string Desc;
	};

}; // end of namespace

