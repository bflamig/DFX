#pragma once

#include <iostream>
#include <vector>
#include <string_view>
#include <algorithm>
#include <unordered_map>
#include <memory>
#include <functional>

enum class UnitEnum
{
    DB,
    DBm,
    PPM,
    Percent,
    Ratio,

    Degree,
    Radian,

    Ohm,
    Farad,
    Henry,

    Volt,
    Vpeak,
    Vrms,
    Vpp,

    Amp,
    Apeak,
    Arms,
    App,

    Coulomb,
    Cpeak,
    Crms,
    Cpp,

    Watt,

    DegreeK,
    DegreeC,
    DegreeF,

    RadiansPerSec,
    Hertz,

    None
};

enum class UnitCatEnum
{
    Ratio, Angle, Resistance, Capacitance, Inductance, Voltage, Current, Charge, Power, Temperature, Frequency, None
};

const std::vector<UnitCatEnum> UnitCats
{
    UnitCatEnum::Ratio, UnitCatEnum::Ratio, UnitCatEnum::Ratio, UnitCatEnum::Ratio, UnitCatEnum::Ratio,
    UnitCatEnum::Angle, UnitCatEnum::Angle,
    UnitCatEnum::Resistance,
    UnitCatEnum::Capacitance,
    UnitCatEnum::Inductance,
    UnitCatEnum::Voltage, UnitCatEnum::Voltage, UnitCatEnum::Voltage, UnitCatEnum::Voltage,
    UnitCatEnum::Current, UnitCatEnum::Current, UnitCatEnum::Current, UnitCatEnum::Current,
    UnitCatEnum::Charge, UnitCatEnum::Charge, UnitCatEnum::Charge, UnitCatEnum::Charge,
    UnitCatEnum::Power,
    UnitCatEnum::Temperature, UnitCatEnum::Temperature, UnitCatEnum::Temperature,
    UnitCatEnum::Frequency, UnitCatEnum::Frequency,
    UnitCatEnum::None
};

template<UnitCatEnum C> bool IsCat(UnitEnum x) { return UnitCats[int(x)] == C; }
template<UnitCatEnum C> double Convert(double old_val, UnitEnum old_u, UnitEnum new_u);


#if 0

template<class C, class T>
auto contains(const C& v, const T& x)
-> decltype(end(v), true)
{
    return end(v) != std::find(begin(v), end(v), x);
}

#endif

extern const std::unordered_map<std::string_view, UnitEnum> mytable;

extern UnitEnum MatchUnits(std::string_view sv);

enum class MetricPrefixEnum
{
    Femto,
    Pico,
    Nano,
    Micro,
    Milli,
    None,
    Kilo,
    Mega,
    Giga,
    Tera,
    Peta,
    Count
};

static constexpr std::string_view MetricPrefixes[] = { "femto", "pico", "nano", "micro", "milli", "", "kilo", "Mega", "Giga", "Tera", "Peta" };

static constexpr std::string_view MetricPrefixMonikers[]{ "f", "p", "n", "u", "m", "", "k", "M", "G", "T", "P" };

static constexpr char MetricPrefixMonikerChars[]{ 'f', 'p', 'n', 'u', 'm', '\0', 'k', 'M', 'G', 'T', 'P' };

static constexpr int MetricExps[]{ -5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5 };

static constexpr int Exps[] = { -15, -12, -9, -6, -3, 0, 3, 6, 9, 12, 15 };

static constexpr double ConvFactors[] = { 1.0e-15, 1.0e-12, 1.0e-9, 1.0e-6, 1.0e-3, 1.0, 1.0e+3, 1.0e+6, 1.0e+9, 1.0e+12, 1.0e+15 };

extern MetricPrefixEnum MatchMetricPrefix(std::string_view sv);

// //////////////////////////////////////////////////////////
// Unit class: Support for different categories of units
// //////////////////////////////////////////////////////////


class Unit {
public:

    UnitEnum unit;
    MetricPrefixEnum metric_prefix;

    Unit(UnitEnum unit_, MetricPrefixEnum metric_prefix_ = MetricPrefixEnum::None)
        : unit(unit_)
        , metric_prefix(metric_prefix_)
    {

    }

    virtual ~Unit() { }

    static std::shared_ptr<Unit> ClassFactory(UnitEnum x);

public:

    static constexpr double peta(double v)
    {
        return v * 1.0e+15;
    }

    static constexpr double tera(double v)
    {
        return v * 1.0e+12;
    }

    static constexpr double giga(double v)
    {
        return v * 1.0e+9;
    }

    static constexpr double mega(double v)
    {
        return v * 1.0e+6;
    }

    static constexpr double kilo(double v)
    {
        return v * 1.0e+3;
    }

    static constexpr double none(double v)
    {
        return v;
    }

    static constexpr double milli(double v)
    {
        return v * 1.0e-3;
    }

    static constexpr double micro(double v)
    {
        return v * 1.0e-6;
    }

    static constexpr double nano(double v)
    {
        return v * 1.0e-9;
    }

    static constexpr double pico(double v)
    {
        return v * 1.0e-12;
    }

    static constexpr double femto(double v)
    {
        return v * 1.0e-15;
    }

public:

    bool IsRatio() { return IsCat<UnitCatEnum::Ratio>(unit); }
    bool IsAngle() { return IsCat<UnitCatEnum::Angle>(unit); }
    bool IsResistance() { return IsCat<UnitCatEnum::Resistance>(unit); }
    bool IsCapacitance() { return IsCat<UnitCatEnum::Capacitance>(unit); }
    bool IsInductance() { return IsCat<UnitCatEnum::Inductance>(unit); }
    bool IsVoltage() { return IsCat<UnitCatEnum::Voltage>(unit); }
    bool IsCurrent() { return IsCat<UnitCatEnum::Current>(unit); }
    bool IsCharge() { return IsCat<UnitCatEnum::Charge>(unit); }
    bool IsPower() { return IsCat<UnitCatEnum::Power>(unit); }
    bool IsTemperature() { return IsCat<UnitCatEnum::Temperature>(unit); }
    bool IsFrequency() { return IsCat<UnitCatEnum::Frequency>(unit); }

    double ConvFactor() const
    {
        return ConvFactors[static_cast<int>(metric_prefix)];
    }
};


template<> double Convert<UnitCatEnum::Ratio>(double old_val, UnitEnum old_u, UnitEnum new_u);

class RatioUnit : public Unit {
public:
    RatioUnit(UnitEnum unit_);
    double ConvertTo(double old_val, UnitEnum new_u)
    {
        // Must be ratio units
        return Convert<UnitCatEnum::Ratio>(old_val, unit, new_u);
    }
};

template<> double Convert<UnitCatEnum::Angle>(double old_val, UnitEnum old_u, UnitEnum new_u);

class AngleUnit : public Unit {
public:
    AngleUnit(UnitEnum unit_);
    double ConvertTo(double old_val, UnitEnum new_u)
    {
        // Must be angle units
        return Convert<UnitCatEnum::Angle>(old_val, unit, new_u);
    }
};


class ResistanceUnit : public Unit {
public:
    ResistanceUnit(UnitEnum unit_);
};

class CapacitanceUnit : public Unit {
public:
    CapacitanceUnit(UnitEnum unit_);
};

class InductanceUnit : public Unit {
public:
    InductanceUnit(UnitEnum unit_);
};

template<> double Convert<UnitCatEnum::Voltage>(double old_val, UnitEnum old_u, UnitEnum new_u);

class VoltageUnit : public Unit {
public:
    VoltageUnit(UnitEnum unit_);
    double ConvertTo(double old_val, UnitEnum new_u)
    {
        // Must be voltage units
        return Convert<UnitCatEnum::Voltage>(old_val, unit, new_u);
    }
};

template<> double Convert<UnitCatEnum::Current>(double old_val, UnitEnum old_u, UnitEnum new_u);

class CurrentUnit : public Unit {
public:
    CurrentUnit(UnitEnum unit_);
    double ConvertTo(double old_val, UnitEnum new_u)
    {
        // Must be current units
        return Convert<UnitCatEnum::Current>(old_val, unit, new_u);
    }
};

template<> double Convert<UnitCatEnum::Charge>(double old_val, UnitEnum old_u, UnitEnum new_u);

class ChargeUnit : public Unit {
public:
    ChargeUnit(UnitEnum unit_);
    double ConvertTo(double old_val, UnitEnum new_u)
    {
        // Must be charge units
        return Convert<UnitCatEnum::Charge>(old_val, unit, new_u);
    }
};

class PowerUnit : public Unit {
public:
    PowerUnit(UnitEnum unit_);
};

template<> double Convert<UnitCatEnum::Temperature>(double old_val, UnitEnum old_u, UnitEnum new_u);

class TemperatureUnit : public Unit {
public:
    TemperatureUnit(UnitEnum unit_);
    double ConvertTo(double old_val, UnitEnum new_u)
    {
        // Must be temperature units
        return Convert<UnitCatEnum::Temperature>(old_val, unit, new_u);
    }
};

template<> double Convert<UnitCatEnum::Frequency>(double old_val, UnitEnum old_u, UnitEnum new_u);

class FrequencyUnit : public Unit {
public:
    FrequencyUnit(UnitEnum unit_);
    double ConvertTo(double old_val, UnitEnum new_u)
    {
        // Must be frequency units
        return Convert<UnitCatEnum::Frequency>(old_val, unit, new_u);
    }
};


// //////////////////////////////////

typedef double (*Cvf)(double old_val, UnitEnum old_u, UnitEnum new_u);

std::vector<std::function<double(double, UnitEnum, UnitEnum)>> ConvertTable{
    Convert<UnitCatEnum::Ratio>,
    Convert<UnitCatEnum::Angle>,
    nullptr, // R
    nullptr, // C
    nullptr, // I
    Convert<UnitCatEnum::Voltage>,
    Convert<UnitCatEnum::Current>,
    Convert<UnitCatEnum::Charge>,
    nullptr, // P
    Convert<UnitCatEnum::Temperature>,
    Convert<UnitCatEnum::Frequency>
};


