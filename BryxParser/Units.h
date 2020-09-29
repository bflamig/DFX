#pragma once

#include <iostream>
#include <vector>
#include <string_view>
#include <algorithm>
#include <memory>

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


//struct Baby
//{
//    UnitEnum type;
//    UnitCatEnum cat;
//};

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

const std::vector<UnitEnum> RatioSet
{
    UnitEnum::DB, UnitEnum::DBm, UnitEnum::PPM, UnitEnum::Percent, UnitEnum::Ratio
};

const std::vector<UnitEnum> AngleSet
{
    UnitEnum::Degree, UnitEnum::Radian
};

const std::vector<UnitEnum> ResistanceSet
{
    UnitEnum::Ohm
};

const std::vector<UnitEnum> CapacitanceSet
{
    UnitEnum::Farad
};

const std::vector<UnitEnum> InductanceSet
{
    UnitEnum::Henry
};

const std::vector<UnitEnum> VoltageSet
{
    UnitEnum::Volt, UnitEnum::Vpeak, UnitEnum::Vrms, UnitEnum::Vpp
};

const std::vector<UnitEnum> CurrentSet
{
    UnitEnum::Amp, UnitEnum::Apeak, UnitEnum::Arms, UnitEnum::App
};

const std::vector<UnitEnum> ChargeSet
{
    UnitEnum::Coulomb, UnitEnum::Cpeak, UnitEnum::Crms, UnitEnum::Cpp
};

const std::vector<UnitEnum> PowerSet
{
    UnitEnum::Watt
};

const std::vector<UnitEnum> TemperatureSet
{
    UnitEnum::DegreeK, UnitEnum::DegreeC, UnitEnum::DegreeF
};

#endif


#if 0

template<class C, class T>
auto contains(const C& v, const T& x)
-> decltype(end(v), true)
{
    return end(v) != std::find(begin(v), end(v), x);
}

#endif

const std::vector<std::vector<std::string_view>> UnitNames =
{
    { "db", "db", "db"},
    { "dbm", "dbm", "dbm" },
    { "ppm", "ppm", "ppm" },
    { "%", "percent", "percent" },
    { "X", "ratio", "ratio" },
    { "deg", "degree", "degrees" },
    { "rad", "radian", "radians" },
    { "R", "Ohm", "Ohms" },
    { "F", "Farad", "Farads"},
    { "H", "Henry", "Henries"},
    { "V", "Volt", "Volts"},
    { "Vpeak", "Volts Peak", "Volts Peak"},
    { "Vpp", "Volts PP", "Volts PP"},
    { "Vrms", "Volts Rms", "Volts Rms"},
    { "A", "Amp", "Amps"},
    { "Apeak", "Amps Peak", "Amps Peak"},
    { "App", "Amps PP", "Amps PP"},
    { "C", "Coulomb", "Coulombs"},
    { "Cpeak", "Coulombs Peak", "Coulombs Peak"},
    { "Cpp", "Coulombs PP", "Coulombs PP"},
    { "W", "Watt", "Watts"},
    { "degK", "degrees K", "degrees K"},
    { "degC", "degrees C", "degrees C"},
    { "degF", "degrees F", "degrees F"},
    { "rps", "radians/sec", "radians/sec"},
    { "Hz", "Hertz", "Hertz"}
};

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
