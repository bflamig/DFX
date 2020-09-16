#pragma once
#include <string>
#include <memory>
#include <exception>
#include <cmath>
#include "EngrVarFlags.h"
//#include "decimal.h"

#if 0
namespace RcdUtils
{
	/// /////////////////////////////////////////////////////////////////////
	/// EngrVar: 
	/// 
	/// A class suitable for storing engineering values with "units." 
	/// (In this base class, the "units" are just the metric prefix.)
	///
	/// We store mantissa and exponent separately. Mantissa is stored as a 
	/// decimal type to facilitate reliable reading/writing round-tripping. 
	/// NOTE: When normalized, Abs(mantissa) less than 1000.
	/// The exponent is actually stored in two parts: The "engineering" exponent
	/// (also called metric exp here) is stored as a power of a thousand (
	/// (or million for squared units) to support metric-style engineering
	/// notation. We also keep track of an additional power of ten exponent 
	/// which is used when the engr exp would otherwise be out of range,
	/// or when we wish to display coerced prefixes. These two exponents are
	/// combined when converting to raw double values.
	/// 
	/// For full unit support, check out some of the derived classes, 
	/// which may or may not have metric prefixes. Also, some of the
	/// functions here let you pass in the base units (like "Volts")
	/// for formatting purposes.


	enum class Units
	{
		// @@ TBD. Just so we can compile
		dB,
		X,
		Hz,
		Ohms,
		Farad,
		Henries,
		Meter,
		Seconds
	};

	enum class UnitChoices
	{
		// @@ TBD. Just so we can compile
		dB,
		X,
		Hz,
		Ohms,
		Farad,
		Henries,
		Meter,
		Seconds
	};

#if 0

	struct decimal 
	{
		std::string digits; // Could include decimal point too

		decimal() { clear(); }

		decimal(int num)
		{
		}

		decimal &operator=(int num)
		{
			// @@ TODO
		}

		bool operator!=(const decimal& other) const
		{
			return digits == other.digits;
		}

		void clear() { digits.clear(); digits.push_back('0');  }

		std::string ToString()
		{
			return digits;
		}
	};

#endif

#if 1


	/// <summary>
	/// The EngrVar base class.
	/// </summary>

	class EngrVar { // : IComparable<EngrVar>

	public:

		// @@ NOTE: Despite the name, the mantissa below can very much have a decimal point. It's not your father's mantissa.
		decimal mantissa;            // We store the mantissa with the decimal type, to alleviate rounding problems of double. Abs(m) < 1000 when normalized.
		int engr_exp;                // We store the exponent in powers of a thousand or million (to support metric-style engineering notation)
		const int MEXP_MULT;         // The multiplier for the engr_exp. Either 3 for "thousands" or 6 for "millions". The latter is for squared units.
		int tens_exp;                // Additional tens exponent that is sometimes tacked on to mantissa (think prefix tracking)
		int error_code;              // If non-zero, then the number is invalid for reasons indicated in the code.
		EngrVarFlags value_flag;     // Always check this first before using the number. We indicate infinity and NaN here.

		// The default way to represent selected units is to base them
		// on the engr_exp variable (aka metric exponents) of EngrNum.
		// So in this form, there is no unit "type" -- it's outside
		// the purview here.

#if 0

		virtual Enum SelectedUnits
		{
			get
			{
				return Units.ExpToPrefix(engr_exp);
			}

			set
			{
				int zyborg = Units.GetMetricExp((Units.Metric)value);

				if (EngrExpInRange(zyborg))
				{
					engr_exp = zyborg;
					ToDouble(); // Sets codes, etc.
				}
				else
				{
					throw new Exception("selected units out of range");
				}
			}
		}
#endif

		// We provide name support for our engineering variables.
		// This is to facilitate storing these babies in files and
		// having a convenient way to name them.

		std::string Name;

		// ////////////////////////////////////////////////////////////////
		// Okay, on to the main class members, starting with various ways
		// of constructing numbers.
		//	 

		// But first, our destructor

		virtual ~EngrVar()
		{
		}

		// In this first constructor, we provide a way to set the 
		// metric multiplier to something other than three

		explicit EngrVar(std::string Name_, int MEXP_MULT_ = 3) // , EngrVar &dmy_)
		: Name(Name_)
		, mantissa()
		, engr_exp(0)
		, MEXP_MULT(MEXP_MULT_)
		, tens_exp(0)
		, error_code(0)
		, value_flag(EngrVarFlags::Ordinary)
		{
			// The dmy variable is there to disambiguate the
			// constructor call. You can pass in null.
		}

		// The first ways involve passing in numbers in both double floating
		// point and decimal.

		EngrVar(std::string Name_, double v)
			: EngrVar(Name_)
		{
			SetNum(v);
		}

		EngrVar(std::string Name_, double v, int power_of_ten_exp)
			: EngrVar(Name_)
		{
			SetNum(v, power_of_ten_exp);
		}

		EngrVar(std::string Name_, decimal m, int power_of_ten_exp)
			: EngrVar(Name_)
		{
			Normalize(m, power_of_ten_exp);
		}

		EngrVar(std::string Name_, decimal m)
			: EngrVar(Name_, m, 0)
		{
		}

		EngrVar(std::string Name_, double v, Units selected_units_)
			: EngrVar(Name_)
		{
			SetNum(v, selected_units_);
		}

		EngrVar(std::string Name_, decimal v, Units selected_units_)
			: EngrVar(Name_)
		{
			SetNum(v, selected_units_);
		}


		// You can also pass in the numbers as text to be parsed. In the parsing
		// you can pass in optional units. The unit description chosen will be
		// set in the UnitName object itself.

		EngrVar(std::string Name_, std::string s, UnitChoices units)
			: EngrVar(Name_) // We *must* initialize for engr_exp logic to work correctly during parsing
		{
			Parse(s, units);
		}

		//
		// Okay, we also want a copy constructor.
		//

		EngrVar(const EngrVar& other)
		: Name(other.Name)
		, mantissa(other.mantissa)
		, engr_exp(other.engr_exp)
		, MEXP_MULT(other.MEXP_MULT)
		, tens_exp(other.tens_exp)
		, error_code(other.error_code)
		, value_flag(other.value_flag)
		{
		}

		//
		// And a move constructor.
		//

		EngrVar(const EngrVar&& other)
		: Name(other.Name)
		, mantissa(std::move(other.mantissa))
		, engr_exp(other.engr_exp)
		, MEXP_MULT(other.MEXP_MULT)
		, tens_exp(other.tens_exp)
		, error_code(other.error_code)
		, value_flag(other.value_flag)
		{
		}


		//
		// And a virtual copy constructor
		//

		virtual std::unique_ptr<EngrVar> Clone()
		{
			return std::make_unique<EngrVar>(*this);
		}

		//
		// And a copy assignment operator
		//

		EngrVar& operator=(const EngrVar& other)
		{
			if (this != &other)
			{
				Name = other.Name;
				mantissa = other.mantissa;
				engr_exp = other.engr_exp;
				// @@ TODO ASSERT: MEXP_MULT = other.MEXP_MULT;
				tens_exp = other.tens_exp;
				error_code = other.error_code;
				value_flag = other.value_flag;
			}
			return *this;
		}

		//
		// And a move assignment operator
		//

		EngrVar& operator=(const EngrVar&& other)
		{
			if (this != &other)
			{
				Name = other.Name;
				mantissa = std::move(other.mantissa);
				engr_exp = other.engr_exp;
				// @@ TODO ASSERT: MEXP_MULT = other.MEXP_MULT;
				tens_exp = other.tens_exp;
				error_code = other.error_code;
				value_flag = other.value_flag;
			}
			return *this;
		}


		//
		// And a SaveTo function. Note: We don't save the name.
		// That defeats the whole purpose of copying.
		// Also, it's ASSUMED the metric exponent multipliers are the same!
		//

		virtual void SaveTo(EngrVar &dest)
		{
			// We must check the type at run-time

			Type this_type = this.GetType();
			Type dest_type = dest.GetType();

			if (dest_type == this_type)
			{
				if (MEXP_MULT != dest.MEXP_MULT)
				{
					throw std::exception("Invalid cast: Metric exponent multipliers are different in EngrVar.SaveTo()");
				}

				dest.mantissa = mantissa;
				dest.engr_exp = engr_exp;
				dest.tens_exp = tens_exp;
				dest.error_code = error_code;
				dest.value_flag = value_flag;
			}
			else if (dest_type == typeof(VoltVar))
			{
				// Really bad hack, looking for a derived class.
				// But this is a special beast.
				double v = ToNativeRaw();
				auto d = (VoltVar)dest;
				d.RawX = v;
			}
			else throw std::exception("Invalid cast: Wrong dest type in EngrVar.SaveTo()");
		}

		//
		// And a diff function
		//

		virtual bool IsSyntacticallyDifferent(const EngrVar &en)
		{
			// if (en.Name != Name) return true;
			if (en.mantissa != mantissa) return true;
			if (en.engr_exp != engr_exp) return true;
			if (en.MEXP_MULT != MEXP_MULT) return true;
			if (en.tens_exp != tens_exp) return true;
			if (en.error_code != error_code) return true;
			if (en.value_flag != value_flag) return true;
			return false;
		}


		virtual bool IsSemanticallyDifferent(const EngrVar &en)
		{
			EngrVar a_num(*this);
			EngrVar b_num(en);

			a_num.Normalize();
			b_num.Normalize();

			return a_num.IsSyntacticallyDifferent(b_num);
		}

		// /////////////////////////////////////////////////////////
		// Some more higher level type functions for dealing with
		// "raw" number semantics.
		// /////////////////////////////////////////////////////////

		virtual std::unique_ptr<EngrVar> FromNativeRaw(double src)
		{
			// We make a new var from src.
			// Easy peasy in this base class version
			return std::make_unique<EngrVar>(Name, src);
		}

		virtual double ToNativeRaw()
		{
			// Take our current value, and convert it into
			// the dest units, and then return the double.
			// In this base version, things are easy,
			// since our units are carried in engr_exp,
			// and the native form is simply the double
			// we get by converting.

			return ToDouble();
		}

		//
		// This property is for convenience
		//

		double RawX()
		{
			return ToNativeRaw(); // This call is virtual
		}

		void SetRawX(double value)
		{
			// @@ BUG FIX: 122214: There was a nasty bug here, where I wasn't
			// copying the values into this number. This is some really ugly
			// coding here. I don't like it at all. And I'm amazed I didn't
			// discover the bug before.
			auto tv = FromNativeRaw(value); // This call is virtual
			tv->SaveTo(*this); // Forgot to do the equivalent of this before!
		}

		// /////////////////////////////////////////////////////////////
		// Public, higher level functions for setting the units on
		// the number.

		// This one is used for combo box relative tracking of units.
		// That is, we make the main part track to different units.
		// Semantically, the RawX output does not change.

		virtual void SetRelativeToUnits(GenericEnum new_u)
		{
			// In this default version, the old and new units carry metric prefix
			// information, and that's all we care about here. The old units are
			// carried implicitly in the engr_exp of the number n.

			auto new_pfx = static_cast<Units::Metric>(new_u);
			int new_mexp = Units::GetMetricExp(new_pfx);
			SetRelativeToEngrExp(new_mexp);
		}

		// Here, we're going to force new units, leaving the
		// rest of the number alone.

		virtual void SetUnits(GenericEnum new_u)
		{
			// In this default version, we are bluntly -- and ultimately,
			// setting the metric_exp of the number.

			auto new_pfx = static_cast<Units::Metric>(new_u);
			int new_mexp = Units::GetMetricExp(new_pfx);
			SetEngrExp(new_mexp);
		}

		// //////////////////////////////////////////////////////////////
		//
		// Public conversion from double and decimal stuff
		//
		// WARNING: These Set() functions are mostly for initalization.
		// (EG: in the constructors, but possibly outside as well.)
		// Don't use these Set functions during RawX type calculations
		// they are not appropriate for them.
		//
		// //////////////////////////////////////////////////////////////

		virtual void SetZero()
		{
			mantissa.clear();
			engr_exp = 0;
			tens_exp = 0;
			error_code = 0;
			value_flag = EngrVarFlags::Ordinary;
		}

		virtual void SetNum(double src, GenericEnum src_u)
		{
			// Default behavior

			auto pfx = static_cast<Units::Metric>(src_u);
			int mexp = Units::GetMetricExp(pfx);
			SetNum(src, mexp * MEXP_MULT);
		}

		virtual void SetNum(decimal src, GenericEnum src_u)
		{
			// Default behavior

			auto pfx = static_cast<Units::Metric>(src_u);
			int mexp = Units::GetMetricExp(pfx);
			SetNum(src, mexp * MEXP_MULT);
		}

	protected:

		// //////////////////////////////////////////////////////////////////
		//
		// Protected conversion from double and decimal stuff, using
		// lower forms of SetNum. I think it's too dangerous and misleading
		// to expose these to the outside. That's because derived classes
		// may handle units differently and need to shield stuff from
		// outside access.
		//
		// Note: Using RawX often better and easier than doing these anyway.
		//
		// /////////////////////////////////////////////////////////////////

		void SetNum(double v)
		{
			if (Double.IsNaN(v))
			{
				value_flag = EngrVarFlags::NaN;
			}
			else if (Double.IsPositiveInfinity(v))
			{
				value_flag = EngrVarFlags::PositiveInfinity;
			}
			else if (Double.IsNegativeInfinity(v))
			{
				value_flag = EngrVarFlags::NegativeInfinity;
			}
			else
			{
				int delta_exp;
				int metric_exp = GetEngrExp(v, delta_exp, MEXP_MULT);
				ConvertFromInner(v, metric_exp, delta_exp);
			}
		}

		// 
		// Why didn't I think of this form before?
		//

		void SetNum(double v, Units::Metric metric_pfx)
		{
			int metric_exp = Units::GetMetricExp(metric_pfx);
			SetNum(v, metric_exp * MEXP_MULT);
		}

		// We made this protected. I think it's too dangerous to expose to
		// the outside. That's because of the derived EngrVar() classes.
		// What should they do with their units?

		void SetNum(double v, int assoc_tens_exp)
		{
			// @@ TO DO: Could this result in wrongful calls in derived classes?

			if (Double.IsNaN(v))
			{
				value_flag = EngrVarFlags::NaN;
			}
			else if (Double.IsPositiveInfinity(v))
			{
				value_flag = EngrVarFlags::PositiveInfinity;
			}
			else if (Double.IsNegativeInfinity(v))
			{
				value_flag = EngrVarFlags::NegativeInfinity;
			}
			else
			{
				int delta_exp;
				int metric_exp = GetEngrExp(v, assoc_tens_exp, delta_exp, MEXP_MULT);
				ConvertFromInner(v, metric_exp, delta_exp);
			}
		}

		// We made this protected. I think it's too dangerous to expose to
		// the outside. That's because of the derived EngrVar() classes.
		// What should they do with their units?
		// Note: Using RawX is a better choice.

		void SetNum(decimal v)
		{
			SetNum(v, 0);
		}

		// 
		// Why didn't I think of this form before?
		//

		void SetNum(decimal v, Units::Metric metric_pfx)
		{
			int metric_exp = Units::GetMetricExp(metric_pfx);
			SetNum(v, metric_exp * MEXP_MULT);
		}


		// We made this protected. I think it's too dangerous to expose to
		// the outside. That's because of the derived EngrVar() classes.
		// What should they do with their units?

		void SetNum(decimal v, int assoc_tens_exp)
		{
			int delta_exp;
			int metric_exp = GetEngrExp(v, assoc_tens_exp, delta_exp, MEXP_MULT);
			ConvertFromInner(v, metric_exp, delta_exp);
		}

		// //////////////////////////////////////////////////////////////
		// We provide a way to normalize a number already stored 
		// This one here is public.
		// //////////////////////////////////////////////////////////////

		void Normalize()
		{
			if (value_flag == EngrVarFlags::Ordinary)
			{
				Normalize(mantissa, tens_exp + engr_exp * MEXP_MULT);
			}
		}

		// 
		// Low level
		//

		void Normalize(decimal m, int additional_tens_exp_)
		{
			int delta_tens_exp;

			engr_exp = GetEngrExp(m, additional_tens_exp_, delta_tens_exp, MEXP_MULT);
			mantissa = Adjust(m, delta_tens_exp);

			// We'd rather not have a tens_exp if we can get away with it.
			// But we must keep the engr exp in range (eg: from femto to Peta)

			KeepEngrExponentInRange();

			ToDouble(); // Checks for validity, sets flags, etc
		}

	private:

		// //////////////////////////////////////////////////////////////
		//
		// Okay, going even lower into the muck
		//
		// //////////////////////////////////////////////////////////////

		void ConvertFromInner(double v, int metric_exp, int delta_exp)
		{
			engr_exp = metric_exp;

			// Keep the engr exp in range (eg: from femto to Peta)

			KeepEngrExponentInRange();

			try
			{
				double w = Adjust(v, delta_exp);
				mantissa = Convert.ToDecimal(w);
			}
			catch (OverflowException)
			{
				error_code = 10;
				value_flag = EngrVarFlags::PositiveInfinity;
				mantissa = 0;
				engr_exp = 0;
				tens_exp = 0;
			}
			catch (Exception)
			{
				error_code = 9;
				value_flag = EngrVarFlags::NaN;
				mantissa = 0;
				engr_exp = 0;
				tens_exp = 0;
			}
		}

		void ConvertFromInner(decimal v, int metric_exp, int delta_exp)
		{
			engr_exp = metric_exp;

			// Keep the engr exp in range (eg: from femto to Peta)

			KeepEngrExponentInRange();

			try
			{
				decimal w = Adjust(v, delta_exp);
				mantissa = w;
			}
			catch (OverflowException)
			{
				error_code = 10;
				value_flag = EngrVarFlags::PositiveInfinity;
				mantissa = 0;
				engr_exp = 0;
				tens_exp = 0;
			}
			catch (Exception)
			{
				error_code = 9;
				value_flag = EngrVarFlags::NaN;
				mantissa = 0;
				engr_exp = 0;
				tens_exp = 0;
			}
		}

		static double Adjust(double v, int delta_exp)
		{
			if (delta_exp > 0)
			{
				while (delta_exp > 0)
				{
					v *= 10.0;
					--delta_exp;
				}
			}
			else if (delta_exp < 0)
			{
				while (delta_exp < 0)
				{
					v /= 10.0;
					++delta_exp;
				}
			}

			return v;
		}

		static decimal Adjust(decimal v, int delta_exp)
		{
			if (delta_exp > 0)
			{
				while (delta_exp > 0)
				{
					v *= 10;
					--delta_exp;
				}
			}
			else if (delta_exp < 0)
			{
				while (delta_exp < 0)
				{
					v /= 10;
					++delta_exp;
				}
			}

			return v;
		}

		static int GetEngrExp(double din, int &delta_exp, int MEXP_MULT)
		{
			// Okay, then ...

			double dabs = std::abs(din);

			double dlog = std::log10(dabs);

			int exp;

			if (dlog < 0)
			{
				exp = (int)std::floor(dlog);
			}
			else exp = (int)std::ceil(dlog);

			bool exact_multiple_of_ten = ((int)dlog == exp);

			return GetEngrExpInner(exp, exact_multiple_of_ten, delta_exp, MEXP_MULT);
		}

		static int GetEngrExp(double din, int additional_tens_exp, int& delta_exp, int MEXP_MULT)
		{
			// @@ BUG: Broken for {101.0  -6}, but works for {0.101, -6}
			// BUG FIX 12/10/2014

			// Okay, then ...

			double dabs = std::abs(din);

			double dlog = std::log10(dabs) + additional_tens_exp; // Bug fix: add it here

			int exp;

			if (dlog < 0)
			{
				exp = (int)std::floor(dlog);
			}
			else exp = (int)std::ceil(dlog);

			bool exact_multiple_of_ten = ((int)dlog == exp);

			int metric_exp = GetEngrExpInner(exp /* bug fix: not here: + additional_tens_exp */, exact_multiple_of_ten, delta_exp, MEXP_MULT);

			delta_exp += additional_tens_exp;

			return metric_exp;
		}

		static int GetEngrExp(decimal m, int additional_tens_exp, int& delta_exp, int MEXP_MULT)
		{
			// @@ BUG: Broken for {101.0m  -6}, but works for {0.101m, -6}
			// BUG FIX 12/10/2014

			// Okay, then ...

			double dabs = std::abs((double)m);

			double dlog = std::log10(dabs) + additional_tens_exp; // bug fix: add it here

			int exp;

			if (dlog < 0)
			{
				exp = (int)std::floor(dlog);
			}
			else exp = (int)std::ceil(dlog);

			bool exact_multiple_of_ten = ((int)dlog == exp);

			int metric_exp = GetEngrExpInner(exp /* bug fix: not here: + additional_tens_exp */, exact_multiple_of_ten, delta_exp, MEXP_MULT);

			delta_exp += additional_tens_exp;

			return metric_exp;
		}

		static int GetEngrExpInner(int exp, bool exact_multiple_of_ten, int& delta_exp, int MEXP_MULT)
		{
			// Default things

			int metric_exp = 0;

			int dexp = exp;

			if (exp == 0)
			{
				// Do nothing
			}
			else if (exp > 0)
			{
				// For positive exponents, our mantissa will either be an exact multiple of 10 or not.

				if (exact_multiple_of_ten)
				{
					// Our mantissa will be an exact multiple of 10 (including 10^0 == 1)

					//    z = 0, 3, 6, 9, 12, 15, ...
					// or z = 0, 6, 12, 18, 24, 30, ...
					int zebra = 0;

					while (true)
					{
						if (exp < zebra) break; // Note: "<" rather than "<="
						dexp = exp - zebra;
						metric_exp = zebra;
						zebra += MEXP_MULT;
					}
				}
				else
				{
					// ?? // Our mantissa would be less than one. We want to effectively multiply it by ten

					//    z = 0, 3, 6, 9, 12, 15, ...
					// or z = 0, 6, 12, 18, 24, 30, ...

					int zebra = 0;

					while (true)
					{
						if (exp <= zebra) break;  // Note: "<=" rather than "<"
						dexp = exp - zebra;
						metric_exp = zebra;
						zebra += MEXP_MULT;
					}
				}
			}
			else
			{
				// Exponent is less than 0

				int pexp = -exp; // Now it becomes positive

				//else if (pexp > 21)
				//{
				//	dexp = 24 - pexp;
				//	metric_exp = -24;
				//}

				int zebra = 0;

				while (pexp > zebra)
				{
					dexp = zebra + MEXP_MULT - pexp;
					metric_exp = -zebra - MEXP_MULT;
					zebra += MEXP_MULT;
				}

			}

			delta_exp = dexp - exp;

			metric_exp /= MEXP_MULT;

			return metric_exp;
		}


		// ///////////////////////////////////////////////////////
		// Even lower level
		// ///////////////////////////////////////////////////////

	protected:


		bool EngrExpInRange(int ee)
		{
			// Default is to just use the standard femto to peta
			return Units::MetricExpInRangeStd(ee);
		}

		void KeepEngrExponentInRange()
		{
			if (!EngrExpInRange(engr_exp))
			{
				tens_exp = engr_exp * MEXP_MULT;
				engr_exp = 0;
			}
			else tens_exp = 0;
		}

		void SetRelativeToEngrExp(int new_engr_exp)
		{
			if (!Units::MetricExpInRangeStd(new_engr_exp))
			{
				throw InvalidOperationException("Metric exponent out of range");
			}

			// Best if we normalize first, and then do our thing. This helps prevent
			// having large mantissas and exponents to boot. It keeps the numbers 
			// more "natural"

			Normalize();

			int tens_shift = (engr_exp - new_engr_exp) * MEXP_MULT;

			tens_exp += tens_shift;

			if (tens_exp >= -6 && tens_exp <= 6)
			{
				mantissa = Adjust(mantissa, tens_exp);
				tens_exp = 0;
			}

			engr_exp = new_engr_exp;

			// Perhaps check for overflow?

			ToDouble(); // This will do the trick. It will trap exceptions and set error codes.
		}

		void SetEngrExp(int new_engr_exp)
		{
			if (!Units::MetricExpInRangeStd(new_engr_exp))
			{
				throw new InvalidOperationException("Metric exponent out of range");
			}

			// Best not to normalize at all. This function is called by the gui, and
			// the user has simply selected a different metric exp. So, whatever you
			// do, don't change the mantissa!
			// Normalize();

			engr_exp = new_engr_exp;

			// Perhaps check for overflow?

			ToDouble(); // This will do the trick. It will trap exceptions and set error codes.
		}

		// /////////////////////////////////////////////////////////////////
		// We need ways of converting to double floating point, so we can
		// do calculations with said numbers.
		// /////////////////////////////////////////////////////////////////

		//
		// This here is somewhat low level. It doesn't know how to map
		// to native space.

	public:

		double ToDouble()
		{
			double v;

			try
			{
				v = ToDoubleInner();
			}
			catch (OverflowException)
			{
				error_code = 10;                            // Just in case
				value_flag = EngrVarFlags::PositiveInfinity; // Just in case
				v = double::PositiveInfinity;
			}
			catch (Exception)
			{
				error_code = 9;                   // Just in case
				value_flag = EngrVarFlags::NaN;    // Just in case
				v = double::NaN;
			}

			return v;
		}

	private:

		double ToDoubleInner()
		{
			// No exceptions are handled here. Use ToDouble() on the outside.
			// Note that we might pass back infinity, NaN, etc.

			double v;

			if (value_flag == EngrVarFlags::Ordinary)
			{
				// possible exception thrown (?)
				v = (double)mantissa;
				v *= std::pow(10.0, tens_exp + engr_exp * MEXP_MULT);
			}
			else if (value_flag == EngrVarFlags::PositiveInfinity)
			{
				v = double::PositiveInfinity;
			}
			else if (value_flag == EngrVarFlags::NegativeInfinity)
			{
				v = Double::NegativeInfinity;
			}
			else
			{
				v = Double::NaN;
			}

			return v;
		}

	public:

		double MainPartToDouble()
		{
			double m = (double)mantissa;
			if (tens_exp != 0)
			{
				m *= std::pow(10.0, tens_exp);
			}
			return m;
		}

		double ScaleToMetric()
		{
			int craycray = -engr_exp * MEXP_MULT;
			double s = std::pow(10.0, craycray);
			return s;
		}

		double SelfScale()
		{
			// self scale
			return MainPartToDouble();
		}

		// ////////////////////////////////////////////////////////////
		//
		// Support for formatting our engineering numbers as strings
		//
		// ////////////////////////////////////////////////////////////

		//
		// First, a few high level calls
		//

		virtual std::string SciFmt(std::string fmt, UnitChoices uc, std::string desc_override = null)
		{
			std::string manpart = MainPart(fmt);
			std::string unit_desc = UnitDescOverride(uc, desc_override);
			return manpart + " " + ShortEngrExpToString() + unit_desc;
		}

		virtual std::string SciFmt(std::string fmt, UnitChoices uc, GenericEnum selected_relative_u, std::string desc_override)
		{
			auto ev = Clone();
			ev->SetRelativeToUnits(selected_relative_u);
			auto rs = ev->SciFmt(fmt, uc, desc_override);
			return rs;
			//string manpart = ev.MainPart(fmt);
			//return manpart + " " + override_desc;
		}

		//
		// And then a bit lower, but still public
		//

		std::string SciFmt(std::string fmt, std::string unit_desc, std::string spacing = " ")
		{
			std::string manpart = MainPart(fmt);
			return manpart + spacing + ShortEngrExpToString() + unit_desc;
		}

		std::string UnitDescOverride(UnitChoices uc, std::string desc_override)
		{
			std::string unit_desc;

			if (uc != null && (desc_override == null || desc_override == "null"))
			{
				// Usually the short version but not always!
				unit_desc = uc.descriptions[uc.display_index];
			}
			else
			{
				unit_desc = desc_override;
			}

			return unit_desc;
		}

		std::string UnitDescOverride(UnitChoices uc, GenericEnum selected_u, std::string desc_override)
		{
			std::string unit_desc;

			if (uc != null && (desc_override == null || desc_override == "null"))
			{
				// Usually the short version but not always!
				unit_desc = uc.UnitDesc(selected_u);
			}
			else
			{
				unit_desc = desc_override;
			}

			return unit_desc;
		}

		std::string MantissaToString(std::string fmt)
		{
			if (fmt == null || fmt == "") fmt = "G29";
			auto result = mantissa.ToString(fmt);
			// Trap for "E+03" and "E-03" and fix up the metric stuff

			int trap = result.IndexOf("E+03");

			if (trap != -1)
			{
				// Fix up by removing the exponent and adjusting the
				// metric exponent accordingly.

				if (engr_exp != 0) // @@ Hmmm ... why trap for this? As a flag for non-metric units?
				{
					engr_exp++;

					// Okay, I give up. How do I simply remove the last four characters from the string?

					result = result.Remove(trap);
				}
			}
			else
			{
				trap = result.IndexOf("E-03");

				if (trap != -1)
				{
					// Fix up by removing the exponent and adjusting the
					// metric exponent accordingly.

					if (engr_exp != 0)  // Hmmm ... Why trap for this? As a flag for non-metric units?
					{
						engr_exp--;

						// Okay, I give up. How do I simply remove the last four characters from the string?

						result = result.Remove(trap);
					}
				}
			}

			return result;
		}

		std::string EngrExpToString()
		{
			if (EngrExpInRange(engr_exp))
			{
				return Units::MetricPrefixes[engr_exp + 5];
			}
			else
			{
				throw exception("Assertion error: EngrNum.EngrExpToString()");
				//return "E" + (engr_exp * MEXP_MULT).ToString();
			}
		}

		std::string ShortEngrExpToString()
		{
			if (EngrExpInRange(engr_exp))
			{
				return Units::MetricPrefixMonikers[engr_exp + 5];
			}
			else
			{
				throw exception("Assertion error: EngrNum.ShortEngrExpToString()");
				//return "E" + (engr_exp * MEXP_MULT).ToString();
			}
		}

		std::string TensExpToString()
		{
			if (tens_exp != 0)
			{
				return "E" + tens_exp.ToString();
			}
			else return "";
		}

		virtual std::string MainPart(std::string fmt = null)
		{
			if (value_flag == EngrVarFlags::NaN)
			{
				return "NaN";
			}
			else if (value_flag == EngrVarFlags::NegativeInfinity)
			{
				return "-Infinity";
			}
			else if (value_flag == EngrVarFlags::PositiveInfinity)
			{
				return "Infinity";
			}
			else
			{
				// The mantissa plus any tens exponent that's currently set
				return MantissaToString(fmt) + TensExpToString();
			}
		}

		void PrepareParts(std::string& MantissaStr, std::string& ExpStr)
		{
			if (EngrExpInRange(engr_exp))
			{
				MantissaStr = MantissaToString(null) + TensExpToString();
				ExpStr = ShortEngrExpToString();
			}
			else
			{
				throw exception("Assertion error: EngrNum.PrepareParts()");
			}
		}

	private:

		static string[] error_desc =
		{ "no error",
			"invalid start character", "decimal point or digit expected", "expecting exponent digits",
			"unexpected character", "too many spaces", "too many group digits",
			"must have three group digits", "expected group digit", "unhandled exception",
			"numeric overflow", "incorrect or unrecognized units"
		};

	public:

		static std::string ErrorCodeToString(int ec)
		{
			if (ec >= 1 && ec <= 11)
			{
				return error_desc[ec];
			}
			else if (ec == 0) return "";
			else return "Unspecified error(" + ec + ")";
		}

		std::string ErrorString()
		{
			return ErrorCodeToString(error_code);
		}


		// //////////////////////////////////////////////////////////////////////////////////////////////////
		// Parsing helpers

	private:

		static decimal ToDecimal(char c)
		{
			return c - '0';
		}

		static bool IsExponentPrefix(char c)
		{
			return c == 'e' || c == 'E';
		}

	public:

		virtual void RecordUnits(int matching_units_index)
		{
			// This function is called by the parser when a matching units suffix is found.
			// The EngrNum base class does nothing with this, but derived classes might.
		}

		// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//
		// Parser for number with either power of ten suffix (e or E) or a engr scale suffix (ie. k, M, G, f, p, n, etc),
		// or both! (Power of ten exponent can precede metric prefix, but not vice versa. Also, % is supported,
		// but only by itself.) Units tacked onto the end are attempted to be parsed. Error code set if no matching
		// units are found.
		//
		// Note: The parser returns the character index to the character right after the last valid character parsed.
		//       If you are not expecting leftovers, then this should point to the null byte.
		// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		int Parse(std::string s, UnitChoices units)
		{
			return Parse(s, units, false); // true = favor_mantissa_as_is
		}

			int Parse(std::string s, UnitChoices units, bool favor_mantissa_as_is, bool digits_only = false)
			{
				// We check for null or empty string. In this case, we'll return a zero

				if (s == null || s.length == 0)
				{
					mantissa = 0;
					engr_exp = 0;
					tens_exp = 0;
					value_flag = EngrVarFlags::Ordinary;
					error_code = 0;
					return 0;
				}

				// Okay, initialize parser state

				value_flag = EngrVarFlags::Ordinary; // ordinary until otherwise stated I guess

				int state = 0;                      // we use a state machine, and start with state 0

				bool is_positive = true;            // mantissa positive flag
				bool exponent_is_positive = true;   // exponent positive flag
				bool at_least_one_digit = false;    // was at least one digit encountered?

				int metric_exp = engr_exp;          // initialize this to current engr exp. Note: 0 means no scaling. @@ WARNING: Be sure to initialize engr_exp if called from constructor!

				tens_exp = 0;                       // We'd rather not have this if we can get away with it. So we reset it right now. It might be set again later.

				decimal v = 0;                      // whole part of mantissa
				decimal f = 0;                      // fractional part of mantissa
				decimal e = 0;                      // power of ten exponent

				int num_tens_digits = 0;            // how many digits to the left of the decimal point have we found, not including leading zeroes

				bool ? is_multiple_of_ten = null;    // is the number we have an exact multiple of ten?
				int multiple_of_ten = 0;            // then what multiple are we?

				int num_group_digits = 0;           // We allow groups of three separated by commas. This is a counter to keep track.
				bool have_groups = false;           // And a boolean to say, yes, we have groups

				int index_to_first_sig_tenth_digit = 0;  // Where is first non-zero fractional digit? 1 means first character to the right of the decimal point

				int num_tenths_digits = 0;               // How many tenths digits have we encountered (not including trailing zeroes ?)

				decimal fractional_divisor = 1;          // Used when processing digits to the right of the decimal point. It goes 1, 10, 100, 1000, etc

				// p is cursor into the string, of length slen

				int p = 0;
				int slen = s.length;

				error_code = 0;

				bool quit = false;

				// Okay, here we go, with our giant state machine.

				try
				{
					while (error_code == 0 && !quit)
					{
						char c = p < slen ? s[p] : '\0'; // Gets set to null byte if at end. This should trigger parser to quit or give an error if expecting more characters

						switch (state)
						{
							case 0: // Look for '+', '-', '.', digit
							{
								if (c == '+')
								{
									is_positive = true;
									state = 100;
									++p;
								}
								else if (c == '-')
								{
									is_positive = false;
									state = 100;
									++p;
								}
								else if (c == '.')
								{
									state = 100;
								}
								else if (Char.IsDigit(c))
								{
									state = 200;
								}
								else
								{
									error_code = 1; // invalid start character
								}
							}
							break;

							case 100: // Look for a digit or a '.'
							{
								if (c == '.')
								{
									++p;
									state = 500; // So we have no tens digits. Look for first fraction digit.
								}
								else if (Char.IsDigit(c))
								{
									state = 200; // First tens digits
								}
								else
								{
									error_code = 2; // decimal or digit expected
								}
							}
							break;

							case 200: // look for first non-zero tens digit, spinning through leading zeroes. We also allow commas every three digits (first group can have 1-3 digits)
							{
								if (Char.IsDigit(c))
								{
									at_least_one_digit = true;
									num_group_digits++;

									v = ToDecimal(c);

									if (v != 0)
									{
										if (v == 1)
										{
											is_multiple_of_ten = true;
											multiple_of_ten = 0;
										}
										else // if (v != 1)
										{
											is_multiple_of_ten = false;
										}

										num_tens_digits = 1;

										++p;
										state = 400; // accumulate tens digits
									}
									else
									{
										num_tens_digits = 0; // still have leading zeroes

										++p;
										// stay on current state
									}
								}
								else if (c == ',')
								{
									if (digits_only)
									{
										--p;
										quit = true;
									}
									else
									{
										if (have_groups == false)
										{
											// Then this is first group. It can have from 1 to 3 digits, even if digits are zeros, I guess.

											if (num_group_digits < 1 || num_group_digits > 3)
											{
												error_code = 6; // Too many group digits
											}
											else ++p; // Skip over valid comma
										}
										else
										{
											// Not the first group. Must have three digits

											if (num_group_digits != 3)
											{
												error_code = 7; // Must have 3 digits
											}
											else ++p; // Skip over valid comma
										}


										num_group_digits = 0; // reset counter
										have_groups = true;

										// Leave state alone
									}
								}
								else
								{
									if (have_groups && num_group_digits != 3)
									{
										error_code = 7; // must have three digits per group
									}
									else state = 300; // there are no non-zero tens digits, so maybe there's a decimal pt, or exponent
								}
							}
							break;

							case 300: // Look for decimal pt, or exponent indicator. For the latter, must have had at least one digit.
							{
								// We get here if all there was were leading zeros (and maybe commas), (or no digits at all).

								if (c == '.')
								{
									++p;
									state = 500; // look for first fraction digit
								}
								else
								{
									if (at_least_one_digit)
									{
										state = 700; // look for exponent indicator
									}
									else error_code = 3; // expecting exponent digits
								}
							}
							break;

							case 400: // Accumulate tens digits till we find non-digit. We can have a comma every three digits (first group can have 1-3 digits)
							{
								if (Char.IsDigit(c))
								{
									num_group_digits++;

									v *= 10;
									decimal d = ToDecimal(c);
									v = v + d;
									// handle leading zeroes
									if (num_tens_digits == 0)
									{
										if (v != 0)
										{
											num_tens_digits = 1;
										}
									}
									else ++num_tens_digits;

									// handle multiples of ten

									if (d == 0)
									{
										if (is_multiple_of_ten == true)
										{
											++multiple_of_ten;
										}
									}
									else if (d == 1)
									{
										if (is_multiple_of_ten == null)
										{
											is_multiple_of_ten = true;
											multiple_of_ten = 0;
										}
										else is_multiple_of_ten = false;
									}
									else is_multiple_of_ten = false;

									// keep state at 400
									++p;
								}
								else if (c == ',')
								{
									if (digits_only)
									{
										--p;
										quit = true;
									}
									else
									{
										if (have_groups == false)
										{
											// This is first group. It can have from 1 to 3 digits, even if the digits are zeroes, I guess.

											if (num_group_digits < 1 || num_group_digits > 3)
											{
												error_code = 6; // Too many group digits
											}
											else ++p; // skip over valid comma
										}
										else
										{
											// Not the first group. Must have three digits

											if (num_group_digits != 3)
											{
												error_code = 7; // Must have 3 digits
											}
											else ++p; // skip over valid comma
										}

										num_group_digits = 0; // reset counter
										have_groups = true;

										// Leave state alone
									}
								}
								else if (c == '.')
								{
									state = 500;  // Look for first fraction digit
									++p;
								}
								else
								{
									state = 700; // Off to exponent part
								}
							}
							break;

							case 500: // First fraction digit
							{
								if (Char.IsDigit(c))
								{
									fractional_divisor = 10;
									decimal d = ToDecimal(c);
									f = d / fractional_divisor;
									num_tenths_digits = 1;

									if (d == 0)
									{

									}
									else
									{
										index_to_first_sig_tenth_digit = num_tenths_digits;

										if (d == 1)
										{
											if (is_multiple_of_ten == null)
											{
												is_multiple_of_ten = true;
												multiple_of_ten = -num_tenths_digits;
											}
											else is_multiple_of_ten = false;
										}
										else is_multiple_of_ten = false;
									}

									++p;
									state = 600;  // accumulate fraction digits
								}
								else
								{
									state = 700;  // off to exponent part
								}

							}
							break;

							case 600: // accumulate fraction digits till first non-digit
							{
								if (Char.IsDigit(c))
								{
									fractional_divisor *= 10;
									decimal d = ToDecimal(c);

									f = f + d / fractional_divisor;
									++num_tenths_digits;

									if (d == 0)
									{
									}
									else
									{
										if (index_to_first_sig_tenth_digit == 0)
										{
											index_to_first_sig_tenth_digit = num_tenths_digits;
										}

										if (d == 1)
										{
											if (is_multiple_of_ten == null)
											{
												is_multiple_of_ten = true;
												multiple_of_ten = -num_tenths_digits;
											}
											else is_multiple_of_ten = false;

										}
										else is_multiple_of_ten = false;
									}

									++p;
									// keep at state at 600;
								}
								else
								{
									state = 700; // off to exponent part
								}
							}
							break;

							case 700: // Look for an optional single space character between mantissa and exponent (%, metric, or tens)
							{
								if (digits_only)
								{
									--p;
									quit = true;
								}
								else
								{
									if (c == ' ')
									{
										++p;
									}

									state = 750; // On to exponents
								}
							}
							break;

							case 750: // Almost ready to look for possible exponent, but must close out last three-digit group
							{
								if (have_groups && num_group_digits != 3)
								{
									error_code = 7; // must have three group digits
								}
								else if (have_groups && num_group_digits == 0)
								{
									error_code = 8; // expected group digit
								}
								else state = 775; // now to actually look for exponent
							}
							break;

							case 775: // look for possible exponent or metric prefix or units. And at this point, no more space characters allowed
							{
								if (c == ' ')
								{
									error_code = 5; // too many spaces
								}
								else if (c == '%') // we'll treat '%' like e-2
								{
									state = 950; // Handle '%' prefix
								}
								else if (IsExponentPrefix(c))
								{
									++p;
									state = 800; // Handle 'e' or 'E' prefix
								}
								else
								{
									int match_len = 0;

									int mp = UnitChoices::IsMetricPrefix(s, p, match_len);

									if (mp != 0)
									{
										state = 975; // handle metric prefix and/or units
									}
									else
									{
										// We don't have a metric prefix, but some units might follow. What shouldn't
										// follow is any other punctuation or symbol.

										if (Char.IsPunctuation(c) || Char.IsSymbol(c))
										{
											error_code = 4; // invalid character encountered
										}
										else
										{
											state = 975; // off to look for units
										}
									}
								}

							}
							break;

							case 800: // looking for optional sign char of exponent
							{
								if (c == '+')
								{
									exponent_is_positive = true;
									++p;
									state = 850; // expect exponent digit
								}
								else if (c == '-')
								{
									exponent_is_positive = false;
									++p;
									state = 850; /// expect exponent digit
								}
								else state = 850; // Expecting an exponent digit
							}
							break;

							case 850: // *Expect* the first exponent digit (we had an exponent indicator, so there should be a following digit)
							{
								if (Char.IsDigit(c))
								{
									e = ToDecimal(c);
									++p;
									state = 900;
								}
								else
								{
									error_code = 3; // expecting exponent digit
								}
							}
							break;

							case 900: // Accumulate exponent digits till we find non-digit. We allow a % sign or metric prefix and/or units to follow the exponent.
							{
								if (Char.IsDigit(c))
								{
									e *= 10;
									e = e + ToDecimal(c);
									++p;
									// keep at current state;
								}
								else
								{
									// We quit scanning for digits if we don't find a character we recognize as part of a number.

									// Update: Disallow any punctuation or other symbols at this point.
									// Exception: We allow a % sign. We also allow a metric prefix.

									//int match_len = 0;

									if (c == '%')
									{
										state = 950; // handle % sign
									}
									//else if (UnitChoices.IsMetricPrefix(s, p, ref match_len) != 0)
									//{
									//    state = 975; // Handle metric prefix and/or units
									//}
									else if (Char.IsPunctuation(c) || Char.IsSymbol(c))
									{
										error_code = 4; // invalid character encountered
									}
									else
									{
										// At this point we've successfully parsed a tens exponent.
										// So we want to clear any metric exp that might have been
										// already set. (You'll get the opportunity to give it a new one in
										// state 975.)

										// Update: No we don't! (Think coerced...) So don't do this: metric_exp = 0;

										state = 975; // look for units or metric prefix
									}
								}
							}
							break;

							case 950: // Experiment: Handle final ending percent sign or metric prefix and/or units.
							{
								if (c == '%')
								{
									++p;

									// At this point we've successfully parsed a percent sign. So it makes no sense to
									// have a metric exponent already set. So clear it. (It may get added back in a bit)

									metric_exp = 0;

									// Subtract 2 from the exponent. Some funny math follows because we don't have a fully
									// constructed tens exponent. The sign of the exponent is separate from the value at this point. So ...

									if (exponent_is_positive)
									{
										if (e < 2)
										{
											e -= 2;
											e = -e; // Uh, wha?
											exponent_is_positive = false;
										}
										else
										{
											e -= 2;
										}
									}
									else
									{
										e += 2;
									}

									std::string rest = s.substr(p);

									if (rest != "")
									{
										error_code = 4; // Invalid character found
									}
									else
									{
										quit = true; // Since we don't allow anything to follow, we're done
									}
								}
								else
								{
									state = 975; // look for metric prefix and/or units
								}
							}
							break;

							case 975: // We are looking for metric prefix (and /or units), but we'll allow a space character beforehand
							{
								if (c == ' ')
								{
									++p;
								}
								state = 980;
							}
							break;

							case 980: // Handle units suffix with possible metric prefix
							{
								int matching_units_index = -1;

								int q = 0;

								std::string rest = s.substr(p);

								if (units != null)
								{
									q = units.ParseUnits(rest, metric_exp, out matching_units_index);
								}

								if (q == 0 && rest != "")
								{
									error_code = 11; // unrecognized units
								}
								else
								{
									p += q;

									rest = s.substr(p);

									if (rest != "")
									{
										error_code = 11; // Unrecognized units
									}
									else
									{
										quit = true;
									}
								}

								if (matching_units_index != -1)
								{
									RecordUnits(matching_units_index); // Record units found, if applicable. (Eg., db/Ratio classes use this).
								}

							}
							break;
						}
					}
				}
				catch (OverflowException /*e*/)
				{
					error_code = 10;
					value_flag = EngrVarFlags::PositiveInfinity;
				}
				catch (Exception /* e */)
				{
					error_code = 9;
					value_flag = EngrVarFlags::NaN;
				}

				//////////////////////////////////////////////
				// Okay, done with the parsing. 
				// Now, we put the pieces together: the whole part of the mantissa, the fraction, the exponent.
				// If normalize_mantissa is true, we adjust things so we are in engineering notation, with |mantissa| < 1000 
				// and exponents powers of thousands. If normalize_mantissa is false, then ... what?

				try
				{
					if (error_code == 0)
					{
						// Now it's time to form the engineering number. Ordinarily, we want |mantissa| < 1000,
						// and a power of thousand exponent, (where engr_exp * MEXP_MULT == actual_power_of_ten_exp). 
						// That's our normal form. However, we might have an engr_exp that's out of metric range.
						// And, the user might have put in a power of ten exponent *along with* a metric exponent.
						// And they might want the mantissa in a non-standard form.

						// First, we must fixup the sign of the exponent

						if (!exponent_is_positive) e = -e;

						// If favor_mantissa_as_is is true, we try to honor the user's intentions and leave the mantissa alone.
						// But we won't honor those intentions if num_tens_digits is > 6, or if/ we have more than 4 leading
						// zeros in the fractional part, or if we have a power of ten exponent. Another issue is if the 
						// metric exp is out of range. That comes into play as well. (We may have to employ the tens exp).

						bool do_the_normalize = !favor_mantissa_as_is;

						if (do_the_normalize == false)
						{
							if (num_tens_digits > 6 || index_to_first_sig_tenth_digit > 5 || e != 0)
							{
								do_the_normalize = true;
							}
						}

						// We may have BOTH power of ten exponent and a metric exp.
						// If so, initially convert to powers of ten exponents and go from there

						if (metric_exp != 0 && e != 0)
						{
							e += metric_exp * MEXP_MULT;
							metric_exp = 0;
						}

						if (e == 0)
						{
							// No power of ten exponent, but might have metric exp (ie. power of thousand exponent).

							if (do_the_normalize == true)
							{
								// Let's see if we're not just a fraction

								if (num_tens_digits > 0)
								{
									// We have a full mantissa
									AdjustFullMantissaWithEngrExponent(v + f, num_tens_digits, metric_exp);
								}
								else
								{
									// We are just a fraction with possible engr exponent
									AdjustFractionalMantissaWithEngrExponent(f, index_to_first_sig_tenth_digit, metric_exp);
								}
							}
							else
							{
								mantissa = v + f;
								engr_exp = metric_exp;
							}

							// We may have to override things if the engr_exp is out of range:

							KeepEngrExponentInRange();
						}
						else  // e != 0
						{
							// We have a powers of ten exponent

							if (num_tens_digits > 0)
							{
								// We have a full mantissa

								int T = num_tens_digits;
								int P = (int)e;

								AdjustFullMantissaWithExponent(v + f, T, P);
							}
							else
							{
								// We have a mantissa that's just a fraction, along with a power of ten exponent

								int T = index_to_first_sig_tenth_digit;
								int P = (int)e;

								AdjustFractionalMantissaWithExponent(f, T, P);
							}

							// We may have to override things if the engr_exp is out of range:

							KeepEngrExponentInRange();
						}

						// Fix up the sign of the mantissa

						if (!is_positive) mantissa = -mantissa;

						// One last thing we'll do: Try to convert our number to a double,
						// and check for any problems, like overflow. This causes flags and
						// codes to be set, etc.

						ToDoubleInner();
					}
					else
					{
						// Invalid parse, so set number to "NaN"

						value_flag = EngrVarFlags::NaN;
					}
				}
				catch (OverflowException /* e */)
				{
					error_code = 10;
					value_flag = EngrVarFlags::PositiveInfinity;
				}
				catch (Exception /* e */)
				{
					error_code = 9;
					value_flag = EngrVarFlags::NaN;
				}

				return p; // Next character in string 
			}

			void AdjustFullMantissaWithEngrExponent(decimal m, int T, int scale_prefix)
			{
				// We have a full mantissa m, with T tens digits, with engr metric prefix given

				//w.Parse("1.0045678");           // 1.0045678     0: -3*(1/3) == 0;   0: 0 + 1/3 == 0
				//w.Parse("10.045678");           // 10.045678     0: -3*(2/3) == 0;   0: 0 + 2/3 == 0
				//w.Parse("100.45678");           // 100.45678     0: -3*(3/3) == 0;   0: 0 + 3/3 == 1  but norem => -3*(2/3) == 0;  0 + 2/3 == 0

				//w.Parse("1004.5678");           // 1.0045678k   -3: -3*(4/3) == -3;  1: 0 + 4/3 == 1
				//w.Parse("10045.678");           // 10.045678k   -3: -3*(5/3) == -3;  1: 0 + 5/3 == 1
				//w.Parse("100456.78");           // 100.45678k   -3: -3*(6/3) == -6;  1: 0 + 6/3 == 2  but norem => -3*(5/3) == -3;  0 + 5/3 == 1

				//w.Parse("1004567.8");           // 1.0045678M   -6: -3*(7/3) == -6;  2: 0 + 7/3 == 2
				//w.Parse("10045678.0");          // 10.045678M   -6: -3*(8/3) == -6;  2: 0 + 8/3 == 2
				//w.Parse("100456780.0");         // 100.45678M   -6: -3*(9/3) == -9;  2: 0 + 9/3 == 3  but norem => -3*(8/3) == -6;  0 + 8/3 == 2

				//w.Parse("1004567800.0");        // 1.0045678G   -9: -3*(10/3) == -9; 3: 0 + 10/3 == 3 


				//w.Parse("1.0045678k");           // 1.0045678k    0: -3*(1/3) == 0;   0: 1 + 1/3 == 1
				//w.Parse("10.045678k");           // 10.045678k    0: -3*(2/3) == 0;   0: 1 + 2/3 == 1
				//w.Parse("100.45678k");           // 100.45678k    0: -3*(3/3) == 0;   0: 1 + 3/3 == 2  but norem => -3*(2/3) == 0;  1 + 2/3 == 1

				//w.Parse("1004.5678k");           // 1.0045678M   -3: -3*(4/3) == -3;  1: 1 + 4/3 == 2
				//w.Parse("10045.678k");           // 10.045678M   -3: -3*(5/3) == -3;  1: 1 + 5/3 == 2
				//w.Parse("100456.78k");           // 100.45678M   -3: -3*(6/3) == -6;  1: 1 + 6/3 == 3  but norem => -3*(5/3) == -3;  1 + 5/3 == 2

				//w.Parse("1004567.8k");           // 1.0045678G   -6: -3*(7/3) == -6;  2: 1 + 7/3 == 3
				//w.Parse("10045678.0k");          // 10.045678G   -6: -3*(8/3) == -6;  2: 1 + 8/3 == 3
				//w.Parse("100456780.0k");         // 100.45678G   -6: -3*(9/3) == -9;  2: 1 + 9/3 == 4  but norem => -3*(8/3) == -6;  1 + 8/3 == 3

				//w.Parse("1004567800.0k");        // 1.0045678T   -9: -3*(10/3) == -9; 3: 1 + 10/3 == 4 


				//w.Parse("1.0045678m");           // 1.0045678m    0: -3*(1/3) == 0;  -1: -1 + 1/3 == -1
				//w.Parse("10.045678m");           // 10.045678m    0: -3*(2/3) == 0;  -1: -1 + 2/3 == -1
				//w.Parse("100.45678m");           // 100.45678m    0: -3*(3/3) == 0;  -1: -1 + 3/3 == -1  but norem => -3*(2/3) == 0;  -1 + 2/3 == -1

				//w.Parse("1004.5678m");           // 1.0045678    -3: -3*(4/3) == -3;  0: -1 + 4/3 == 0
				//w.Parse("10045.678m");           // 10.045678    -3: -3*(5/3) == -3;  0: -1 + 5/3 == 0
				//w.Parse("100456.78m");           // 100.45678    -3: -3*(6/3) == -6;  0: -1 + 6/3 == 1  but norem => -3*(5/3) == -3;  -1 + 5/3 == 0

				//w.Parse("1004567.8m");           // 1.0045678k   -6: -3*(7/3) == -6;  1: -1 + 7/3 == 1
				//w.Parse("10045678.0m");          // 10.045678k   -6: -3*(8/3) == -6;  1: -1 + 8/3 == 1
				//w.Parse("100456780.0m");         // 100.45678k   -6: -3*(9/3) == -9;  1: -1 + 9/3 == 2  but norem => -3*(8/3) == -6;  -1 + 8/3 == 1

				//w.Parse("1004567800.0m");        // 1.0045678M   -9: -3*(10/3) == -9; 2: -1 + 10/3 == 2 

				int cowboy = T % MEXP_MULT;
				int indian = T / MEXP_MULT;

				engr_exp = scale_prefix + indian;

				if (cowboy == 0)
				{
					--engr_exp;
					--indian;
				}

				mantissa = Adjust(m, -MEXP_MULT * indian);
			}

			void AdjustFractionalMantissaWithEngrExponent(decimal f, int T, int scale_prefix)
			{
				// we have a fractional mantissa f, T = index to first sig tenth digit, and engr metric prefix as given

				//w.Parse("0.10045678");           // 100.45678m   3: 3*(1 + 1/3) == 3;  -1: -1 - 1 / 3 == -1
				//w.Parse("0.010045678");          // 10.045678m   3: 3*(1 + 2/3) == 3;  -1: -1 - 2 / 3 == -1
				//w.Parse("0.0010045678");         // 1.0045678m   3: 3*(1 + 3/3) == 3;  -1: -1 - 3 / 3 == -2  but norem => 3*(1 + 2/3) == 3;  -1 - 2 / 3 == -1

				//w.Parse("0.00010045678");        // 100.45678u   6: 3*(1 + 4/3) == 6;  -2: -1 - 4 / 3 == -2
				//w.Parse("0.000010045678");       // 10.045678u   6: 3*(1 + 5/3) == 6;  -2: -1 - 5 / 3 == -2
				//w.Parse("0.0000010045678");      // 1.0045678u   6: 3*(1 + 6/3) == 9;  -2: -1 - 6 / 3 == -3  but norem => 3*(1 + 5/3) == 6; -1 - 5 / 3 == -2

				//w.Parse("0.00000010045678");     // 100.45678n   9: 3*(1 + 7/3) == 9;  -3: -1 - 7 / 3 == -3
				//w.Parse("0.000000010045678");    // 10.045678n   9: 3*(1 + 8/3) == 9;  -3: -1 - 8 / 3 == -3
				//w.Parse("0.0000000010045678");   // 1.0045678n   9: 3*(1 + 9/3) == 12; -3: -1 - 9 / 3 == -4  but norem => 3*(1 + 8/3) == 9; -1 - 8 / 3 == -3

				//w.Parse("0.00000000010045678");  // 100.45678p   12: 3*(1 + 10/3) == 12; -4: -1 - 10 / 3 == -4

				//w.Parse("0.10045678k");          // 100.45678    3: 3*(1 + 1/3) == 3;  0: 0 - 1 / 3 ==  0
				//w.Parse("0.010045678k");         // 10.045678    3: 3*(1 + 2/3) == 3;  0: 0 - 2 / 3 ==  0
				//w.Parse("0.0010045678k");        // 1.0045678    3: 3*(1 + 3/3) == 6;  0: 0 - 3 / 3 == -1  but norem => 3*(1 + 2/3) == 3;  0 - 2 / 3 == 0

				//w.Parse("0.00010045678k");       // 100.45678m   6: 3*(1 + 4/3) == 6;  -1: 0 - 4 / 3 == -1
				//w.Parse("0.000010045678k");      // 10.045678m   6: 3*(1 + 5/3) == 6;  -1: 0 - 5 / 3 == -1
				//w.Parse("0.0000010045678k");     // 1.0045678m   6: 3*(1 + 6/3) == 9;  -1: 0 - 6 / 3 == -2  but norem => 3*(1 + 5/3) == 6;  0 - 5 / 3 == -1

				//w.Parse("0.00000010045678k");    // 100.45678u   9: 3*(1 + 7/3) == 9;  -2: 0 - 7 / 3 == -2
				//w.Parse("0.000000010045678k");   // 10.045678u   9: 3*(1 + 8/3) == 9;  -2: 0 - 8 / 3 == -2
				//w.Parse("0.0000000010045678k");  // 1.0045678u   9: 3*(1 + 9/3) == 12; -2: 0 - 9 / 3 == -3  but norem => 3*(1 + 8/3) == 9;  0 - 8 / 3 == -2

				//w.Parse("0.00000000010045678k"); // 100.45678n   12: 3*(1 + 10/3) == 12; -3: 0 - 10 /3 == -3

				//w.Parse("0.10045678M");          // 100.45678k   3: 3*(1 + 1/3) == 3;  1: 1 - 1 / 3 == 1
				//w.Parse("0.010045678M");         // 10.045678k   3: 3*(1 + 2/3) == 3;  1: 1 - 2 / 3 == 1
				//w.Parse("0.0010045678M");        // 1.0045678k   3: 3*(1 + 3/3) == 6;  1: 1 - 3 / 3 == 0  but norem => 3*(1 + 2/3) == 3;  1 - 2 / 3 == 1

				//w.Parse("0.00010045678M");       // 100.45678    6: 3*(1 + 4/3) == 6;  0: 1 - 4 / 3 == 0
				//w.Parse("0.000010045678M");      // 10.045678    6: 3*(1 + 5/3) == 6;  0: 1 - 5 / 3 == 0
				//w.Parse("0.0000010045678M");     // 1.0045678    6: 3*(1 + 6/3) == 9;  0: 1 - 6 / 3 == -1  but norem => 3*(1 + 5/3) == 6;  1 - 5 / 3 == 0

				//w.Parse("0.00000010045678M");    // 100.45678m   9: 3*(1 + 7/3) == 9;  -1: 1 - 7 / 3 == -1
				//w.Parse("0.000000010045678M");   // 10.045678m   9: 3*(1 + 8/3) == 9;  -1: 1 - 8 / 3 == -1
				//w.Parse("0.0000000010045678M");  // 1.0045678m   9: 3*(1 + 9/3) == 12; -1: 1 - 9 / 3 == -2  but norem => 3*(1 + 8/3) == 9;  1 - 8 / 3 == -1

				//w.Parse("0.00000000010045678M"); // 100.45678u   12: 3*(1 + 10/3) == 12;  -2: 0 - 10 /3 == -2

				int base_engr_exp = -1; // milli
				int start_exp = scale_prefix + base_engr_exp;

				int cowboy = T % MEXP_MULT;
				int indian = T / MEXP_MULT;

				engr_exp = start_exp - indian;

				if (cowboy == 0)
				{
					++engr_exp;
					--indian;
				}

				mantissa = Adjust(f, MEXP_MULT * (1 + indian));
			}


			void AdjustFullMantissaWithExponent(decimal m, int T, int P)
			{
				// Mantissa must not just be a fraction here, or this will be wrong

				// T = number of tens digits (with leading zeroes removed)
				// P = power of ten exponent

				////                                                T    P   A   T+P   S        

				//w.Parse("1.0045678e-3");          // 1.0045678m   1   -3   0   -2    0:   0 - 0 ==> 0 - 3*((1+0)/3)
				//w.Parse("10.045678e-3");          // 10.045678m   2   -3   0   -1    0:   0 - 0 ==> 0 - 3*((2+0)/3)
				//w.Parse("100.45678e-3");          // 100.45678m   3   -3   0    0    0:   0 - 0 ==> 0 - 3*((3+0-1)/3)

				//w.Parse("1004.5678e-3");          // 1.0045678    4   -3   0    1   -3:   0 - 3 ==> 0 - 3*((4+0)/3)
				//w.Parse("10045.678e-3");          // 10.045678    5   -3   0    2   -3:   0 - 3 ==> 0 - 3*((5+0)/3)
				//w.Parse("100456.78e-3");          // 100.45678    6   -3   0    3   -3:   0 - 3 ==> 0 - 3*((6+0-1)/3)

				//w.Parse("1004567.8e-3");          // 1.0045678k   7   -3   0    4   -6:   0 - 6 ==> 0 - 3*((7+0)/3)
				//w.Parse("10045678.0e-3");         // 10.045678k   8   -3   0    5   -6:   0 - 6 ==> 0 - 3*((8+0)/3)
				//w.Parse("100456780.0e-3");        // 100.45678k   9   -3   0    6   -6:   0 - 6 ==> 0 - 3*((9+0-1)/3)

				//w.Parse("1004567800.0e-3");       // 1.0045678M   10  -3   0    7   -9:   0 - 9 ==> 0 - 3*((10+0)/3)

				////                                                T    P   A   T+P   S

				//w.Parse("1.0045678e-2");          // 10.045678m   1   -2   1   -1    1:   1 - 0 ==> 1 - 3*((1+1)/3)       
				//w.Parse("10.045678e-2");          // 100.45678m   2   -2   1    0    1:   1 - 0 ==> 1 - 3*((2+1-1)/3)
				//w.Parse("100.45678e-2");          // 1.0045678    3   -2   1    1   -2:   1 - 3 ==> 1 - 3*((3+1)/3)    

				//w.Parse("1004.5678e-2");          // 10.045678    4   -2   1    2   -2:   1 - 3 ==> 1 - 3*((4+1)/3)    
				//w.Parse("10045.678e-2");          // 100.45678    5   -2   1    3   -2:   1 - 3 ==> 1 - 3*((5+1-1)/3)
				//w.Parse("100456.78e-2");          // 1.0045678k   6   -2   1    4   -5:   1 - 6 ==> 1 - 3*((6+1)/3)

				//w.Parse("1004567.8e-2");          // 10.045678k   7   -2   1    5   -5:   1 - 6 ==> 1 - 3*((5+1)/3)
				//w.Parse("10045678.0e-2");         // 100.45678k   8   -2   1    6   -5:   1 - 6 ==> 1 - 3*((8+1-1)/3)
				//w.Parse("100456780.0e-2");        // 1.0045678M   9   -2   1    7   -8:   1 - 9 ==> 1 - 3*((9+1)/3)

				//w.Parse("1004567800.0e-2");       // 10.045678M   10  -2   1    7   -8:   1 - 9 ==> 1 - 3*((10+1)/3)

				////                                                T    P   A   T+P   S

				//w.Parse("1.0045678e-1");          // 100.45678m   1   -1   2    0    2:   2 - 0 ==> 2 - 3*((1+2-1)/3)
				//w.Parse("10.045678e-1");          // 1.0045678    2   -1   2    1   -1:   2 - 3 ==> 2 - 3*((2+2)/3)
				//w.Parse("100.45678e-1");          // 10.045678    3   -1   2    2   -1:   2 - 3 ==> 2 - 3*((3+2)/3)

				//w.Parse("1004.5678e-1");          // 100.45678    4   -1   2    3   -1:   2 - 3 ==> 2 - 3*((4+2-1)/3)
				//w.Parse("10045.678e-1");          // 1.0045678k   5   -1   2    4   -4:   2 - 6 ==> 2 - 3*((5+2)/3)
				//w.Parse("100456.78e-1");          // 10.045678k   6   -1   2    5   -4:   2 - 6 ==> 2 - 3*((6+2)/3)

				//w.Parse("1004567.8e-1");          // 100.45678k   7   -1   2    6   -4:   2 - 6 ==> 2 - 3*((7+2-1)/3)
				//w.Parse("10045678.0e-1");         // 1.0045678M   8   -1   2    7   -7:   2 - 9 ==> 2 - 3*((8+2)/3)
				//w.Parse("100456780.0e-1");        // 10.045678M   9   -1   2    8   -7:   2 - 9 ==> 2 - 3*((9+2)/3)

				//w.Parse("1004567800.0e-1");       // 100.45678M   10  -1   2    9   -7:   2 - 9 ==> 2 - 3*((10+2)/3)

				////                                                T    P   A   T+P   S

				//w.Parse("1.0045678e0");           // 1.0045678    1    0   0    1    0:   0 - 0 ==> 0 - 3*((1+0)/3)
				//w.Parse("10.045678e0");           // 10.045678    2    0   0    2    0:   0 - 0 ==> 0 - 3*((2+0)/3)
				//w.Parse("100.45678e0");           // 100.45678    3    0   0    3    0:   0 - 0 ==> 0 - 3*((3+0-1)/3)

				//w.Parse("1004.5678e0");           // 1.0045678k   4    0   0    4   -3:   0 - 3 ==> 0 - 3*((4+0)/3)
				//w.Parse("10045.678e0");           // 10.045678k   5    0   0    5   -3:   0 - 3 ==> 0 - 3*((5+0)/3)
				//w.Parse("100456.78e0");           // 100.45678k   6    0   0    6   -3:   0 - 3 ==> 0 - 3*((6+0-1)/3)

				//w.Parse("1004567.8e0");           // 1.0045678M   7    0   0    7   -6:   0 - 6 ==> 0 - 3*((7+0)/3)
				//w.Parse("10045678.0e0");          // 10.045678M   8    0   0    8   -6:   0 - 6 ==> 0 - 3*((8+0)/3)
				//w.Parse("100456780.0e0");         // 100.45678M   9    0   0    9   -6:   0 - 6 ==> 0 - 3*((9+0-1)/3)

				//w.Parse("1004567800.0e0");        // 1.0045678G   10   0   0   10   -9:   0 - 9 ==> 0 - 3*((10+0)/3)

				////                                                T    P   A   T+P   S

				//w.Parse("1.0045678e1");           // 10.045678    1    1   1    2    1:   1 - 0 ==> 1 - 3*((1+1)/3)       
				//w.Parse("10.045678e1");           // 100.45678    2    1   1    3    1:   1 - 0 ==> 1 - 3*((2+1-1)/3)       
				//w.Parse("100.45678e1");           // 1.0045678k   3    1   1    4   -2:   1 - 0 ==> 1 - 3*((3+1)/3)       

				//w.Parse("1004.5678e1");           // 10.045678k   4    1   1    5   -2:   1 - 0 ==> 1 - 3*((4+1)/3)       
				//w.Parse("10045.678e1");           // 100.45678k   5    1   1    6   -2:   1 - 0 ==> 1 - 3*((5+1-1)/3)       
				//w.Parse("100456.78e1");           // 1.0045678M   6    1   1    7   -5:   1 - 0 ==> 1 - 3*((6+1)/3)       

				//w.Parse("1004567.8e1");           // 10.045678M   7    1   1    8   -5:   1 - 0 ==> 1 - 3*((7+1)/3)       
				//w.Parse("10045678.0e1");          // 100.45678M   8    1   1    9   -5:   1 - 0 ==> 1 - 3*((8+1-1)/3)       
				//w.Parse("100456780.0e1");         // 1.0045678G   9    1   1   10   -8:   1 - 0 ==> 1 - 3*((9+1)/3)       

				//w.Parse("1004567800.0e1");        // 10.045678G   10   1   1   11   -8:   1 - 0 ==> 1 - 3*((10+1)/3)       

				////                                                T    P   A   T+P   S

				//w.Parse("1.0045678e2");           // 100.45678    1    2   2    3    2:   2 - 0 ==> 2 - 3*((1+2-1)/3)
				//w.Parse("10.045678e2");           // 1.0045678k   2    2   2    4   -1:   2 - 3 ==> 2 - 3*((2+2)/3)
				//w.Parse("100.45678e2");           // 10.045678k   3    2   2    5   -1:   2 - 3 ==> 2 - 3*((3+2)/3)

				//w.Parse("1004.5678e2");           // 100.45678k   4    2   2    6   -1:   2 - 3 ==> 2 - 3*((4+2-1)/3)
				//w.Parse("10045.678e2");           // 1.0045678M   5    2   2    7   -4:   2 - 6 ==> 2 - 3*((5+2)/3)
				//w.Parse("100456.78e2");           // 10.045678M   6    2   2    8   -4:   2 - 6 ==> 2 - 3*((6+2)/3)

				//w.Parse("1004567.8e2");           // 100.45678M   7    2   2    9   -4:   2 - 6 ==> 2 - 3*((7+2-1)/3)
				//w.Parse("10045678.0e2");          // 1.0045678G   8    2   2   10   -7:   2 - 9 ==> 2 - 3*((8+2)/3)
				//w.Parse("100456780.0e2");         // 10.045678G   9    2   2   11   -7:   2 - 9 ==> 2 - 3*((9+2)/3)

				//w.Parse("1004567800.0e2");        // 100.45678G   10   2   2   12   -7:   2 - 9 ==> 2 - 3*((10+2-1)/3)

				////                                                T    P   A   T+P   S

				//w.Parse("1.0045678e3");           // 1.0045678k   1    3   0    4    0:   0 - 0 ==> 0 - 3*((1+0)/3)
				//w.Parse("10.045678e3");           // 10.045678k   2    3   0    5    0:   0 - 0 ==> 0 - 3*((2+0)/3)
				//w.Parse("100.45678e3");           // 100.45678k   3    3   0    6    0:   0 - 0 ==> 0 - 3*((3+0-1)/3)

				//w.Parse("1004.5678e3");           // 1.0045678M   4    3   0    7   -3:   0 - 3 ==> 0 - 3*((4+0)/3)
				//w.Parse("10045.678e3");           // 10.045678M   5    3   0    8   -3:   0 - 3 ==> 0 - 3*((5+0)/3)
				//w.Parse("100456.78e3");           // 100.45678M   6    3   0    9   -3:   0 - 3 ==> 0 - 3*((6+0-1)/3)

				//w.Parse("1004567.8e3");           // 1.0045678G   7    3   0   10   -6:   0 - 6 ==> 0 - 3*((7+0)/3)
				//w.Parse("10045678.0e3");          // 10.045678G   8    3   0   11   -6:   0 - 6 ==> 0 - 3*((8+0)/3)
				//w.Parse("100456780.0e3");         // 100.45678G   9    3   0   12   -6:   0 - 6 ==> 0 - 3*((9+0-1)/3)

				//w.Parse("1004567800.0e3");        // 1.0045678T   10   3   0   13   -9:   0 - 9 ==> 0 - 3*((10+0)/3)

				int TPP = T + P;

				int A;

				if (P >= 0)
				{
					A = P % MEXP_MULT;
				}
				else
				{
					// w.Parse("1.0045678e-6"); // 1.0045678u   1   -6   -3    0:   
					// w.Parse("1.0045678e-5"); // 10.045678u   1   -5   -3    1:   
					// w.Parse("1.0045678e-4"); // 100.45678u   1   -4   -3    2:   
					// w.Parse("1.0045678e-3"); // 1.0045678m   1   -4   -3    0:   
					// w.Parse("1.0045678e-2"); // 10.045678m   1   -4   -3    1:   
					// w.Parse("1.0045678e-1"); // 100.45678m   1   -4   -3    2:   

					// -6 ==> 0     6 % 3 == 0
					// -5 ==> 1     5 % 3 == 2
					// -4 ==> 2     4 % 3 == 1
					// -3 ==> 0     3 % 3 == 0
					// -2 ==> 1     2 % 3 == 2
					// -1 ==> 2     1 % 3 == 1

					A = (-P) % MEXP_MULT;
					if (A != 0) A = MEXP_MULT - A;
				}

				int modTPP = TPP % MEXP_MULT;

				int N = T + A;
				if (modTPP == 0) --N;

				int S = A - MEXP_MULT * (N / MEXP_MULT);

				mantissa = Adjust(m, S);

				//////

				engr_exp = (P - S) / MEXP_MULT;
			}

			void AdjustFractionalMantissaWithExponent(decimal f, int T, int P)
			{
				////                                                   T    P   P-T   S:

				//w.Parse("0.10045678e-3");           // 100.45678u    1   -3   -4    3:
				//w.Parse("0.010045678e-3");          // 10.045678u    2   -3   -5    3:
				//w.Parse("0.0010045678e-3");         // 1.0045678u    3   -3   -6    3:

				//w.Parse("0.00010045678e-3");        // 100.45678n    4   -3   -7    6:
				//w.Parse("0.000010045678e-3");       // 10.045678n    5   -3   -8    6:
				//w.Parse("0.0000010045678e-3");      // 1.0045678n    6   -3   -9    6:

				//w.Parse("0.00000010045678e-3");     // 100.45678p    7   -3  -10    9:
				//w.Parse("0.000000010045678e-3");    // 10.045678p    8   -3  -11    9:
				//w.Parse("0.0000000010045678e-3");   // 1.0045678p    9   -3  -12    9:

				//w.Parse("0.00000000010045678e-3");  // 100.45678f   10   -3  -13   12:

				////                                                   T    P   P-T   S:

				//w.Parse("0.10045678e-2");           // 1.0045678m    1   -2   -3    1:
				//w.Parse("0.010045678e-2");          // 100.45678u    2   -2   -4    4:
				//w.Parse("0.0010045678e-2");         // 10.045678u    3   -2   -5    4:

				//w.Parse("0.00010045678e-2");        // 1.0045678u    4   -2   -6    4:
				//w.Parse("0.000010045678e-2");       // 100.45678n    5   -2   -7    7:
				//w.Parse("0.0000010045678e-2");      // 10.045678n    6   -2   -8    7:

				//w.Parse("0.00000010045678e-2");     // 1.0045678n    7   -2   -9    7:
				//w.Parse("0.000000010045678e-2");    // 100.45678p    8   -2  -10   10:
				//w.Parse("0.0000000010045678e-2");   // 10.045678p    9   -2  -11   10:

				//w.Parse("0.00000000010045678e-2");  // 1.0045678p   10   -2  -12   10:

				////                                                   T    P   P-T   S:

				//w.Parse("0.10045678e-1");           // 10.045678m    1   -1   -2    2:
				//w.Parse("0.010045678e-1");          // 1.0045678m    2   -1   -3    2:
				//w.Parse("0.0010045678e-1");         // 100.45678u    3   -1   -4    5:

				//w.Parse("0.00010045678e-1");        // 10.045678u    4   -1   -5    5:
				//w.Parse("0.000010045678e-1");       // 1.0045678u    5   -1   -6    5:
				//w.Parse("0.0000010045678e-1");      // 100.45678n    6   -1   -7    8:

				//w.Parse("0.00000010045678e-1");     // 10.045678n    7   -1   -8    8:
				//w.Parse("0.000000010045678e-1");    // 1.0045678n    8   -1   -9    8:
				//w.Parse("0.0000000010045678e-1");   // 100.45678p    9   -1  -10   11:

				//w.Parse("0.00000000010045678e-1");  // 10.045678p   10   -1  -11   11:

				////                                                   T    P   P-T   S        

				//w.Parse("0.10045678e0");           // 100.45678m     1    0   -1    3
				//w.Parse("0.010045678e0");          // 10.045678m     2    0   -2    3
				//w.Parse("0.0010045678e0");         // 1.0045678m     3    0   -3    3

				//w.Parse("0.00010045678e0");        // 100.45678u     4    0   -4    6
				//w.Parse("0.000010045678e0");       // 10.045678u     5    0   -5    6
				//w.Parse("0.0000010045678e0");      // 1.0045678u     6    0   -6    6

				//w.Parse("0.00000010045678e0");     // 100.45678n     7    0   -7    9
				//w.Parse("0.000000010045678e0");    // 10.045678n     8    0   -8    9
				//w.Parse("0.0000000010045678e0");   // 1.0045678n     9    0   -9    9

				//w.Parse("0.00000000010045678e0");  // 100.45678p    10    0  -10   12

				////                                                   T    P   P-T   S        

				//w.Parse("0.10045678e1");           // 1.0045678      1    1    0    1
				//w.Parse("0.010045678e1");          // 100.45678m     2    1   -1    4
				//w.Parse("0.0010045678e1");         // 10.045678m     3    1   -2    4

				//w.Parse("0.00010045678e1");        // 1.0045678m     4    1   -3    4
				//w.Parse("0.000010045678e1");       // 100.45678u     5    1   -4    7
				//w.Parse("0.0000010045678e1");      // 10.045678u     6    1   -5    7

				//w.Parse("0.00000010045678e1");     // 1.0045678u     7    1   -6    7
				//w.Parse("0.000000010045678e1");    // 100.45678n     8    1   -7   10
				//w.Parse("0.0000000010045678e1");   // 10.045678n     9    1   -8   10

				//w.Parse("0.00000000010045678e1");  // 1.0045678n    10    1   -9   10 

				////                                                   T    P   P-T   S        

				//w.Parse("0.10045678e2");          // 10.045678       1    2    1    2
				//w.Parse("0.010045678e2");         // 1.0045678       2    2    0    2
				//w.Parse("0.0010045678e2");        // 100.45678m      3    2   -1    5

				//w.Parse("0.00010045678e2");       // 10.045678m      4    2   -2    5
				//w.Parse("0.000010045678e2");      // 1.0045678m      5    2   -3    5
				//w.Parse("0.0000010045678e2");     // 100.45678u      6    2   -4    8

				//w.Parse("0.00000010045678e2");    // 10.045678u      7    2   -5    8
				//w.Parse("0.000000010045678e2");   // 1.0045678u      8    2   -6    8
				//w.Parse("0.0000000010045678e2");  // 100.45678n      9    2   -7   11 

				//w.Parse("0.00000000010045678e2"); // 10.045678n     10    2   -8   11

				////                                                   T    P   P-T   S        

				//w.Parse("0.10045678e3");          // 100.45678       1    3    2    3
				//w.Parse("0.010045678e3");         // 10.045678       2    3    1    3 
				//w.Parse("0.0010045678e3");        // 1.0045678       3    3    0    3

				//w.Parse("0.00010045678e3");       // 100.45678m      4    3   -1    6
				//w.Parse("0.000010045678e3");      // 10.045678m      5    3   -2    6
				//w.Parse("0.0000010045678e3");     // 1.0045678m      6    3   -3    6

				//w.Parse("0.00000010045678e3");    // 100.45678u      7    3   -4    9
				//w.Parse("0.000000010045678e3");   // 10.045678u      8    3   -5    9
				//w.Parse("0.0000000010045678e3");  // 1.0045678u      9    3   -6    9

				//w.Parse("0.00000000010045678e3"); // 100.45678n     10    3   -7   12

				////                                                   T    P   P-T   S        

				//w.Parse("0.10045678e4");          // 1.0045678k      1    4    3    3
				//w.Parse("0.010045678e4");         // 100.45678       2    4    2    3 
				//w.Parse("0.0010045678e4");        // 10.045678       3    4    1    3

				//w.Parse("0.00010045678e4");       // 1.0045678       4    4    0    6
				//w.Parse("0.000010045678e4");      // 100.45678m      5    4   -1    6
				//w.Parse("0.0000010045678e4");     // 10.045678m      6    4   -2    6

				//w.Parse("0.00000010045678e4");    // 1.0045678m      7    4   -3    9
				//w.Parse("0.000000010045678e4");   // 100.45678u      8    4   -4    9
				//w.Parse("0.0000000010045678e4");  // 10.045678u      9    4   -5    9

				//w.Parse("0.00000000010045678e4"); // 100.45678u     10    4   -6   12

				////                                                   T    P   P-T   S        

				//w.Parse("0.10045678e4");          // 1.0045678k      1    4    3    1
				//w.Parse("0.10045678e5");          // 10.045678k      1    5    4    2
				//w.Parse("0.10045678e6");          // 100.45678k      1    6    5    3
				//w.Parse("0.10045678e7");          // 1.0045678M      1    6    6    1
				//w.Parse("0.10045678e8");          // 10.045678M      1    6    7    2

				int effective_exp = P - T;

				// Compute the "engineering exponent: ie. powers of a thousand

				engr_exp = effective_exp / MEXP_MULT;  // Like ceiling?

				// Need to fixup if on a particular boundary

				// Don't fully understand the following. So don't trust it.
				// But it seems to work.

				int modulo_effective_exp = effective_exp % MEXP_MULT;

				if (modulo_effective_exp != 0)
				{
					if (effective_exp <= 0)
					{
						--engr_exp;     // Like floor?
					}
					else
					{
						// Why no adjust here?
					}
				}

				// Calculate amount to shift mantissa.

				// First, the actual new exponent in powers of ten

				int new_effective_exp = engr_exp * MEXP_MULT;

				// diff between old and new

				int diff = new_effective_exp - effective_exp;

				// shift = -(diff - T) ==> -diff + T  ==> T - diff

				int Shift = T - diff;

				mantissa = Adjust(f, Shift);  // NOTE: we assume v = 0 (ie. we're just a fraction)
			}

			// ////////////////////////////////////////////////////////////////////
			//
			// ////////////////////////////////////////////////////////////////////

	public:

			int CompareTo(const EngrVar &other)
			{
				double a = ToDouble();
				double b = other.ToDouble();

				// @@ TODO: Sorts descending. Hacky I know.
				if (a > b) return -1;
				if (a < b) return 1;
				return 0;
			}

	}; // Of class

#endif
}

#endif