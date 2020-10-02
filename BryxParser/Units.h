#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <string_view>
#include <algorithm>
#include <unordered_map>
#include <memory>
#include <functional>
#include "SymTree.h"

namespace bryx
{
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

    struct metric_db_elem
    {
        MetricPrefixEnum prefix;
        std::string moniker;
        std::string full;
        int metric_exp;
        int tens_exp;
        double conversion_factor;
    };

    extern const std::vector<metric_db_elem> metric_db;

    // /////////////////////////////////////////////
    // A search tree useful when metric prefixes

    class MpfxParseTree : public SymTree {
    public:

        MpfxParseTree() { }

        MpfxParseTree(std::vector<metric_db_elem> pfx_list);

        virtual ~MpfxParseTree() = default;

        virtual std::shared_ptr<SymTree> MakeNewTree()
        {
            return std::make_shared<MpfxParseTree>();
        }

        void add_pfxname(std::string_view s, MetricPrefixEnum pfx)
        {
            SymTree::addstring(s, static_cast<int>(pfx));
        }

        MetricPrefixEnum find_pfxname(std::string_view s) const;

        int MetricPrefixIndex(char c) const;

        virtual void print_leaf(std::ostream& sout, int id) const
        {
            sout << metric_db[id].moniker;
        }
    };

    extern const MpfxParseTree mpfx_parse_tree;


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

        Other,
        None
    };

    extern const std::vector<std::string> ShortUnitNames;

    // /////////////////////////////////////////////

    enum class UnitCatEnum
    {
        Ratio, Angle, Resistance, Capacitance, Inductance, Voltage, Current, Charge, Power, Temperature, Frequency, None
    };

    extern const std::vector<UnitCatEnum> UnitCats;

    template<UnitCatEnum C> bool IsCat(UnitEnum x) { return UnitCats[int(x)] == C; }
    template<UnitCatEnum C> double Convert(double old_val, UnitEnum old_u, UnitEnum new_u);

    // /////////////////////////////////////////////

#if 0

    template<class C, class T>
    auto contains(const C& v, const T& x)
        -> decltype(end(v), true)
    {
        return end(v) != std::find(begin(v), end(v), x);
    }

#endif

    // /////////////////////////////////////////////
    // A search tree useful when parsing units

    class UnitParseTree : public SymTree {
    public:

        UnitParseTree() { }

        UnitParseTree(std::initializer_list<std::pair<std::string_view, UnitEnum>> unit_list);

        virtual ~UnitParseTree() = default;

        virtual std::shared_ptr<SymTree> MakeNewTree()
        {
            return std::make_shared<UnitParseTree>();
        }

        void add_unitname(std::string_view s, UnitEnum unit)
        {
            SymTree::addstring(s, static_cast<int>(unit));
        }

        UnitEnum find_unitname(std::string_view s) const
        {
            auto index = SymTree::search(s);

            if (index == -1)
            {
                return UnitEnum::None;
            }
            else return static_cast<UnitEnum>(index);
        }

        virtual void print_leaf(std::ostream& sout, int id) const
        {
            sout << ShortUnitNames[id];
        }
    };

    extern const UnitParseTree unit_parse_tree;


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
            return metric_db[static_cast<int>(metric_prefix)].conversion_factor;
        }

        double ConvertTo(double old_val, UnitEnum new_u);
    };


    template<> double Convert<UnitCatEnum::Ratio>(double old_val, UnitEnum old_u, UnitEnum new_u);
    template<> double Convert<UnitCatEnum::Angle>(double old_val, UnitEnum old_u, UnitEnum new_u);
    template<> double Convert<UnitCatEnum::Voltage>(double old_val, UnitEnum old_u, UnitEnum new_u);
    template<> double Convert<UnitCatEnum::Current>(double old_val, UnitEnum old_u, UnitEnum new_u);
    template<> double Convert<UnitCatEnum::Charge>(double old_val, UnitEnum old_u, UnitEnum new_u);
    template<> double Convert<UnitCatEnum::Temperature>(double old_val, UnitEnum old_u, UnitEnum new_u);
    template<> double Convert<UnitCatEnum::Frequency>(double old_val, UnitEnum old_u, UnitEnum new_u);

} // End of namespace bryx

