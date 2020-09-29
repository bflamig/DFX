#include "Units.h"

static const double sqrt2 = std::sqrt(2);
static const double one_over_sqrt2 = 1.0 / std::sqrt(2);
static const double one_over_two_times_sqrt2 = 1.0 / (2.0 * std::sqrt(2));
static constexpr double pi = 3.14159; // @@ TODO: FIX THIS
static constexpr double two_pi = 2.0 * pi;

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


const std::unordered_map<std::string_view, UnitEnum> mytable = {
    {"db", UnitEnum::DB},
    {"dB", UnitEnum::DB},
    {"dbm", UnitEnum::DBm},
    {"dBm", UnitEnum::DBm},
    {"ppm", UnitEnum::PPM},

    {"%", UnitEnum::Percent},
    {"percent", UnitEnum::Percent},

    {"X", UnitEnum::Ratio},
    {"ratio", UnitEnum::Ratio},

    {"deg", UnitEnum::Degree},
    {"degree", UnitEnum::Degree},
    {"degrees", UnitEnum::Degree},

    {"rad", UnitEnum::Radian},
    {"radian", UnitEnum::Radian},
    {"radians", UnitEnum::Radian},

    {"R", UnitEnum::Ohm},
    {"O", UnitEnum::Ohm},
    {"Ohm", UnitEnum::Ohm},
    {"Ohms", UnitEnum::Ohm},

    {"F", UnitEnum::Farad},
    {"Farad", UnitEnum::Farad},
    {"Farads", UnitEnum::Farad},

    {"H", UnitEnum::Henry},
    {"Henry", UnitEnum::Henry},
    {"Henries", UnitEnum::Henry},

    {"V", UnitEnum::Volt},
    {"Volt", UnitEnum::Volt},
    {"Volts", UnitEnum::Volt},
    {"Vpeak", UnitEnum::Vpeak},
    {"VoltsPeak", UnitEnum::Vpeak},
    {"Vpp", UnitEnum::Vpp},
    {"VoltsPP", UnitEnum::Vpp},
    {"Vrms", UnitEnum::Vrms},
    {"VoltsRms", UnitEnum::Vrms},

    {"A", UnitEnum::Amp},
    {"Amp", UnitEnum::Amp},
    {"Amps", UnitEnum::Amp},
    {"Apeak", UnitEnum::Apeak},
    {"AmpsPeak", UnitEnum::Apeak},
    {"App", UnitEnum::App},
    {"AmpsPP", UnitEnum::App},
    {"Arms", UnitEnum::Arms},
    {"AmpsRms", UnitEnum::Arms},

    {"C", UnitEnum::Coulomb},
    {"Coulomb", UnitEnum::Coulomb},
    {"Coulombs", UnitEnum::Coulomb},
    {"Cpeak", UnitEnum::Cpeak},
    {"CoulombsPeak", UnitEnum::Cpeak},
    {"Cpp", UnitEnum::Cpp},
    {"CoulombsPP", UnitEnum::Cpp},
    {"Crms", UnitEnum::Crms},
    {"CoulombsRms", UnitEnum::Arms},

    {"W", UnitEnum::Watt},
    {"Watt", UnitEnum::Watt},
    {"Watts", UnitEnum::Watt},

    {"degK", UnitEnum::DegreeK},
    {"degreesK", UnitEnum::DegreeK},

    {"degC", UnitEnum::DegreeC},
    {"degreesC", UnitEnum::DegreeC},

    {"degF", UnitEnum::DegreeF},
    {"degreesF", UnitEnum::DegreeF},

    {"rps", UnitEnum::RadiansPerSec},
    {"radians/sec", UnitEnum::RadiansPerSec},

    {"Hz", UnitEnum::Hertz},
    {"Hertz", UnitEnum::Hertz}
};

UnitEnum MatchUnits(std::string_view sv)
{
    try
    {
        auto xx = mytable.at(sv);
        return xx;
    }
    catch (...)
    {
        return UnitEnum::None;
    }
}


// //////////////////////////////////

double Unit::ConvertTo(double old_val, UnitEnum new_u)
{

    auto zebra = UnitCats[int(unit)];
    return ConvertTable[int(zebra)](old_val, unit, new_u);
}

// ///////////////////////////////////

template<> double Convert<UnitCatEnum::Ratio>(double old_val, UnitEnum old_u, UnitEnum new_u)
{
    double new_val;

    if (!IsCat<UnitCatEnum::Ratio>(old_u) || !IsCat<UnitCatEnum::Ratio>(new_u))
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

template<> double Convert<UnitCatEnum::Angle>(double old_val, UnitEnum old_u, UnitEnum new_u)
{
    double new_val;

    if (!IsCat<UnitCatEnum::Angle>(old_u) || !IsCat<UnitCatEnum::Angle>(new_u))
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

// Support for converting between the common voltage types, (peak, peak-to-peak, rms)

template<> double Convert<UnitCatEnum::Voltage>(double old_val, UnitEnum old_u, UnitEnum new_u)
{
    double new_val = old_val;

    if (!IsCat<UnitCatEnum::Voltage>(old_u) || !IsCat<UnitCatEnum::Voltage>(new_u))
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

// Support for converting between the common amperage types, (peak, peak-to-peak, rms)

template<> double Convert<UnitCatEnum::Current>(double old_val, UnitEnum old_u, UnitEnum new_u)
{
    double new_val = old_val;

    if (!IsCat<UnitCatEnum::Current>(old_u) || !IsCat<UnitCatEnum::Current>(new_u))
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

// Support for converting between the common charge types, (peak, peak-to-peak, rms)

template<> double Convert<UnitCatEnum::Charge>(double old_val, UnitEnum old_u, UnitEnum new_u)
{
    double new_val = old_val;

    if (!IsCat<UnitCatEnum::Charge>(old_u) || !IsCat<UnitCatEnum::Charge>(new_u))
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

template<> double Convert<UnitCatEnum::Temperature>(double old_val, UnitEnum old_u, UnitEnum new_u)
{
    double new_val;

    if (!IsCat<UnitCatEnum::Temperature>(old_u) || !IsCat<UnitCatEnum::Temperature>(new_u))
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


// ///////////////////////////////////

template<> double Convert<UnitCatEnum::Frequency>(double old_val, UnitEnum old_u, UnitEnum new_u)
{
    double new_val;

    if (!IsCat<UnitCatEnum::Frequency>(old_u) || !IsCat<UnitCatEnum::Frequency>(new_u))
    {
        throw std::exception("One of the units is not a frequency type");
    }

    double native;  // native is hertz

    switch (old_u)
    {
        case UnitEnum::RadiansPerSec:
        native = old_val / two_pi;
        break;

        case UnitEnum::Hertz:
        default:
        native = old_val;
        break;
    }

    switch (new_u)
    {
        case UnitEnum::RadiansPerSec:
        new_val = two_pi * native;
        break;

        case UnitEnum::Hertz:
        default:
        // already in native form
        new_val = native;
        break;
    }

    return new_val;
}