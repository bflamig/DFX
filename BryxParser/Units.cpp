#include "Units.h"

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
 * into Json files that can be parsed by any software supporting Json syntax.
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


namespace bryx
{
    static const double sqrt2 = std::sqrt(2);
    static const double one_over_sqrt2 = 1.0 / std::sqrt(2);
    static const double one_over_two_times_sqrt2 = 1.0 / (2.0 * std::sqrt(2));
    static constexpr double pi = 3.14159; // @@ TODO: FIX THIS
    static constexpr double two_pi = 2.0 * pi;


    // ///////////////////////////////////////////////////////////
        
    const std::vector<metric_db_elem> metric_db =
    {
        {MetricPrefixEnum::Femto, "f", "femto", -5, -15, 1.0e-15},
        {MetricPrefixEnum::Pico,  "p", "pico",  -4, -12, 1.0e-12},
        {MetricPrefixEnum::Nano,  "n", "nano",  -3, -9,  1.0e-9},
        {MetricPrefixEnum::Micro, "u", "micro", -2, -6,  1.0e-6},
        {MetricPrefixEnum::Milli, "m", "milli", -1, -3,  1.0e-3},
        {MetricPrefixEnum::None,  "",   "",      0,  0,  1.0},
        {MetricPrefixEnum::Kilo,  "k", "kilo",   1,  3,  1.0e3},
        {MetricPrefixEnum::Mega,  "M", "Mega",   2,  6,  1.0e6},
        {MetricPrefixEnum::Giga,  "G", "Giga",   3,  9,  1.0e9},
        {MetricPrefixEnum::Tera,  "T", "Tera",   4,  12, 1.0e12},
        {MetricPrefixEnum::Peta,  "P", "Peta",   5,  15, 1.0e15}
    };


    MpfxParseTree::MpfxParseTree(std::vector<metric_db_elem> pfx_list)
    {
        for (auto& e : pfx_list)
        {
            add_pfxname(e.moniker, e.prefix);
            add_pfxname(e.full, e.prefix);
        }
    }

    MetricPrefixEnum MpfxParseTree::find_pfxname(std::string_view s) const
    {
        auto index = SymTree::search(s);

        if (index == -1)
        {
            return MetricPrefixEnum::None;
        }
        else return static_cast<MetricPrefixEnum>(index);
    }

    int MpfxParseTree::MetricPrefixIndex(char c) const
    {
        auto index = SymTree::find_index(c);

        if (index != -1)
        {
            return children.at(index).id; // The enumerated pfx in integer form
        }
        else return -1;
    }

    const MpfxParseTree mpfx_parse_tree(metric_db);


    // /////////////////////////////////////////////////


    const std::vector<std::string> ShortUnitNames
    {
        "dB",
        "dBm",
        "PPM",
        "%",
        "X",
        "deg",
        "rad",
        "Ohm",
        "F",
        "H",
        "V",
        "Vpeak",
        "Vrms",
        "Vpp",
        "A",
        "Apeak",
        "Arms",
        "App",
        "C",
        "Cpeak",
        "Crms",
        "Cpp",
        "W",
        "degK",
        "degC",
        "degF",
        "rps",
        "Hz",
        "None"
    };

    // ///////////////////////////////////////////////////////////

    UnitParseTree::UnitParseTree(std::initializer_list<std::pair<std::string_view, UnitEnum>> unit_list)
    {
        for (auto& e : unit_list)
        {
            add_unitname(e.first, e.second);
        }
    }

	const UnitParseTree unit_parse_tree
    (
		{
			{"db", UnitEnum::DB},
			{"dB", UnitEnum::DB},
			{"dbm", UnitEnum::DBm},
			{"dBm", UnitEnum::DBm},
			{"ppm", UnitEnum::PPM},

			{"%", UnitEnum::Percent},
			{"percent", UnitEnum::Percent},

			{"X", UnitEnum::SimpleRatio},
			{"ratio", UnitEnum::SimpleRatio},

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
		}
	);

    // //////////////////////////////////

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

    const std::vector<std::function<double(double, UnitEnum, UnitEnum)>> ConvertTable{
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


    double Unit::ConvertTo(double old_val, UnitEnum new_u)
    {

        auto kitty = UnitCats[int(unit)];
        return ConvertTable[int(kitty)](old_val, unit, new_u);
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

            case UnitEnum::SimpleRatio:
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

            case UnitEnum::SimpleRatio:
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


} // end of namespace bryx