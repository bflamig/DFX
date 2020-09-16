#pragma once

// /////////////////////////////////////////////////////////////////////
// flags representing the status of the stored value

enum class EngrVarFlags
{
	Ordinary,           // So, thus, use mantissa and engr_exp
	PositiveInfinity,   // Number too big to represent with a double
	NegativeInfinity,   // Number too small to represent with a double
	NaN                 // Undefined, due to conversion error, etc. 
};

