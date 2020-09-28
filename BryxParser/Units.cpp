#include "Units.h"

static const double sqrt2 = std::sqrt(2);
static const double one_over_sqrt2 = 1.0 / std::sqrt(2);
static const double one_over_two_times_sqrt2 = 1.0 / (2.0 * std::sqrt(2));
static constexpr double pi = 3.14159; // @@ TODO: FIX THIS

MetricPrefixEnum MatchMetricPrefix(std::string_view sv)
{
    // Not optimized at all

    int n = static_cast<int>(MetricPrefixEnum::Count);

    // Check the one letter monikers

    for (int i = 0; i < n; i++)
    {
        if (MetricPrefixMonikers[i] == sv)
        {
            return static_cast<MetricPrefixEnum>(i);
        }
    }

    // Check the full prefix names

    for (int i = 0; i < n; i++)
    {
        if (MetricPrefixes[i] == sv)
        {
            return static_cast<MetricPrefixEnum>(i);
        }
    }

    return MetricPrefixEnum::None;
}


UnitEnum MatchUnits(std::string_view sv)
{
    // Not optimized at all

    int n = static_cast<int>(UnitEnum::None);

    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            if (UnitNames[i][j] == sv)
            {
                return static_cast<UnitEnum>(i);
            }
        }
    }

    return UnitEnum::None;

}


// //////////////////////////////////

// A static function

std::shared_ptr<Unit> Unit::ClassFactory(UnitEnum x)
{
    if (Unit::IsRatio(x))
    {
        return std::make_shared<RatioUnit>(x);
    }
    else if (Unit::IsAngle(x))
    {
        return std::make_shared<AngleUnit>(x);
    }
    else if (Unit::IsResistance(x))
    {
        return std::make_shared<ResistanceUnit>(x);
    }
    else if (Unit::IsCapacitance(x))
    {
        return std::make_shared<CapacitanceUnit>(x);
    }
    else if (Unit::IsInductance(x))
    {
        return std::make_shared<InductanceUnit>(x);
    }
    else if (Unit::IsVoltage(x))
    {
        return std::make_shared<VoltageUnit>(x);
    }
    else if (Unit::IsCurrent(x))
    {
        return std::make_shared<CurrentUnit>(x);
    }
    else if (Unit::IsCharge(x))
    {
        return std::make_shared<ChargeUnit>(x);
    }
    else if (Unit::IsPower(x))
    {
        return std::make_shared<PowerUnit>(x);
    }
    else if (Unit::IsCurrent(x))
    {
        return std::make_shared<CurrentUnit>(x);
    }

    return nullptr;
}


// ///////////////////////////////////



// ///////////////////////////////////


RatioUnit::RatioUnit(UnitEnum unit_)
    : Unit(unit_)
{
    if (!IsRatio())
    {
        throw std::exception("Arg type not a ratio");
    }
}


double RatioUnit::ConvertType(double old_val, UnitEnum old_u, UnitEnum new_u)
{
    double new_val;

    if (!Unit::IsRatio(old_u) || !Unit::IsRatio(new_u))
    {
        throw std::exception("One of the units is not a ratio type");
    }

    double ratio;

    switch (old_u)
    {
        case UnitEnum::DB:
        ratio = pow(10.0, old_val / 20.0);
        break;

        case UnitEnum::DBm:
        // Going from dBm to mW
        ratio = pow(10.0, old_val / 10.0);
        // mW to Watts
        ratio /= 1000.0;
        break;

        case UnitEnum::PPM:
        ratio = old_val * 1.0e-6;
        break;

        case UnitEnum::Percent:
        ratio = old_val / 100.0;
        break;

        case UnitEnum::Ratio:
        default:
        ratio = old_val;
        break;
    }


    switch (new_u)
    {
        case UnitEnum::DB:
        new_val = 20.0 * log10(ratio);
        break;

        case UnitEnum::DBm:
        // Watts to mW
        ratio *= 1000.0;
        // Going from mW to dBm
        new_val = 10.0 * log10(ratio);
        break;

        case UnitEnum::PPM:
        new_val = ratio * 1.0e+6;
        break;

        case UnitEnum::Percent:
        new_val = ratio * 100.0;
        break;

        case UnitEnum::Ratio:
        default:
        new_val = ratio;
        break;
    }

    return new_val;
}



// ///////////////////////////////////

AngleUnit::AngleUnit(UnitEnum unit_)
    : Unit(unit_)
{
    if (!IsAngle())
    {
        throw std::exception("Arg type not an angle");
    }
}

double AngleUnit::ConvertType(double old_val, UnitEnum old_u, UnitEnum new_u)
{
    double new_val;

    if (!Unit::IsAngle(old_u) || !Unit::IsAngle(new_u))
    {
        throw std::exception("One of the units is not an angle type");
    }

    double native;  // native is radians

    switch (old_u)
    {
        case UnitEnum::Degree:
            native = old_val * pi / 180.0;
        break;
        case UnitEnum::Radian:
        default:
            native = old_val;
        break;
    }

    switch (new_u)
    {
        case UnitEnum::Degree:
            new_val = native * 180.0 / pi;
        break;
        case UnitEnum::Radian:
        default:
            new_val = native;
        break;
    }

    return new_val;
}


// ///////////////////////////////////

ResistanceUnit::ResistanceUnit(UnitEnum unit_)
    : Unit(unit_)
{
    if (!IsResistance())
    {
        throw std::exception("Arg type not a resistance");
    }
}

// ///////////////////////////////////

CapacitanceUnit::CapacitanceUnit(UnitEnum unit_)
    : Unit(unit_)
{
    if (!IsCapacitance())
    {
        throw std::exception("Arg type not a capacitance");
    }
}

// ///////////////////////////////////

InductanceUnit::InductanceUnit(UnitEnum unit_)
    : Unit(unit_)
{
    if (!IsInductance())
    {
        throw std::exception("Arg type not an inductance");
    }
}


// ///////////////////////////////////

VoltageUnit::VoltageUnit(UnitEnum unit_)
    : Unit(unit_)
{
    if (!IsVoltage())
    {
        throw std::exception("Arg type not a voltage");
    }
}



// Support for converting between the common voltage types, (peak, peak-to-peak, rms)

double VoltageUnit::ConvertType(double old_val, UnitEnum old_u, UnitEnum new_u)
{
    double new_val = old_val;

    if (!Unit::IsVoltage(old_u) || !Unit::IsVoltage(new_u))
    {
        throw std::exception("One of the units is not a voltage type");
    }


    if (old_u == UnitEnum::Vpeak)
    {
        switch (new_u)
        {
            case UnitEnum::Vpp:
            new_val = 2.0 * old_val;
            break;
            case UnitEnum::Vrms:
            new_val = old_val * one_over_sqrt2;
            break;
        }
    }
    else if (old_u == UnitEnum::Vpp)
    {
        switch (new_u)
        {
            case UnitEnum::Vpeak:
            new_val = old_val / 2.0;
            break;
            case UnitEnum::Vrms:
            new_val = old_val * one_over_two_times_sqrt2;
            break;
        }

    }
    else // old_vu == Units.VoltEnum.Vrms
    {
        switch (new_u)
        {
            case UnitEnum::Vpeak:
            new_val = old_val * sqrt2;

            break;
            case UnitEnum::Vpp:
            new_val = old_val * 2.0 * sqrt2;
            break;

        }
    }

    return new_val;
}


// ///////////////////////////////////

CurrentUnit::CurrentUnit(UnitEnum unit_)
    : Unit(unit_)
{
    if (!IsCurrent())
    {
        throw std::exception("Arg type not a current");
    }
}

// Support for converting between the common amperage types, (peak, peak-to-peak, rms)

double CurrentUnit::ConvertType(double old_val, UnitEnum old_u, UnitEnum new_u)
{
    double new_val = old_val;

    if (!Unit::IsCurrent(old_u) || !Unit::IsCurrent(new_u))
    {
        throw std::exception("One of the units is not a current type");
    }


    if (old_u == UnitEnum::Apeak)
    {
        switch (new_u)
        {
            case UnitEnum::App:
            new_val = 2.0 * old_val;
            break;
            case UnitEnum::Arms:
            new_val = old_val * one_over_sqrt2;
            break;
        }
    }
    else if (old_u == UnitEnum::App)
    {
        switch (new_u)
        {
            case UnitEnum::Apeak:
            new_val = old_val / 2.0;
            break;
            case UnitEnum::Arms:
            new_val = old_val * one_over_two_times_sqrt2;
            break;
        }

    }
    else // old_u == Units.AmpEnum.Arms
    {
        switch (new_u)
        {
            case UnitEnum::Apeak:
            new_val = old_val * sqrt2;

            break;
            case UnitEnum::App:
            new_val = old_val * 2.0 * sqrt2;
            break;

        }

    }

    return new_val;
}


// ///////////////////////////////////

ChargeUnit::ChargeUnit(UnitEnum unit_)
    : Unit(unit_)
{
    if (!IsCharge())
    {
        throw std::exception("Arg type not a charge");
    }
}

// Support for converting between the common charge types, (peak, peak-to-peak, rms)

double ChargeUnit::ConvertType(double old_val, UnitEnum old_u, UnitEnum new_u)
{
    double new_val = old_val;

    if (!Unit::IsCharge(old_u) || !Unit::IsCharge(new_u))
    {
        throw std::exception("One of the units is not a charge type");
    }

    if (old_u == UnitEnum::Cpeak)
    {
        switch (new_u)
        {
            case UnitEnum::Cpp:
            new_val = 2.0 * old_val;
            break;
            case UnitEnum::Crms:
            new_val = old_val * one_over_sqrt2;
            break;
        }
    }
    else if (old_u == UnitEnum::Cpp)
    {
        switch (new_u)
        {
            case UnitEnum::Cpeak:
            new_val = old_val / 2.0;
            break;
            case UnitEnum::Crms:
            new_val = old_val * one_over_two_times_sqrt2;
            break;
        }

    }
    else // old_u == Units.ChargeEnum.Crms
    {
        switch (new_u)
        {
            case UnitEnum::Cpeak:
            new_val = old_val * sqrt2;

            break;
            case UnitEnum::Cpp:
            new_val = old_val * 2.0 * sqrt2;
            break;

        }

    }

    return new_val;
}


// ///////////////////////////////////

PowerUnit::PowerUnit(UnitEnum unit_)
    : Unit(unit_)
{
    if (!IsPower())
    {
        throw std::exception("Arg type not a power");
    }
}

// ///////////////////////////////////

TemperatureUnit::TemperatureUnit(UnitEnum unit_)
    : Unit(unit_)
{
    if (!IsTemperature())
    {
        throw std::exception("Arg type not a temperature");
    }
}

double TemperatureUnit::ConvertType(double old_val, UnitEnum old_u, UnitEnum new_u)
{
    double new_val;

    if (!Unit::IsTemperature(old_u) || !Unit::IsTemperature(new_u))
    {
        throw std::exception("One of the units is not a temperature type");
    }

    double native;  // native is degrees C

    switch (old_u)
    {
        case UnitEnum::DegreeK:
            native = old_val - 273.15;
        break;

        case UnitEnum::DegreeC:
        default:
            native = old_val;
        break;

        case UnitEnum::DegreeF:
            native = (old_val - 32.0) * 5.0 / 9.0;
        break;
    }

    switch (new_u)
    {
        case UnitEnum::DegreeK:
            new_val = native + 273.15;
        break;

        case UnitEnum::DegreeC:
        default:
            // already in native form
            new_val = native;
        break;

        case UnitEnum::DegreeF:
            new_val = (native * 9.0 / 5.0) + 32.0;
        break;
    }

    return new_val;
}