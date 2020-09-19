#pragma once

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

// NOTE: The following code is left over from another project, whose features 
// will be slowly added over time. Right now, it's in a rough, beginning state
// and as such is mostly commented out for now. Think of this file as a placeholder.


#include <string>
#include <string_view>
#include <vector>
#include <cmath>
#include <memory>
//#include "ValDesc.h"

namespace bryx
{
	struct ValDesc; // forw decl

	class Units {
	public:

		enum class RatioDBEnum
		{
			ratio,
			dB
		};

		enum class RatioDBmEnum
		{
			mW,
			dBm
		};

		enum class RatioPPMEnum
		{
			ratio,
			PPM
		};

		enum class PercentDBEnum
		{
			percent,
			dB
		};

		enum class PercentNumEnum
		{
			percent,
			num
		};

		enum class AngleEnum
		{
			radians,
			degrees
		};

		enum class VoltEnum
		{
			// Must be in this order for unit choices display index to work properly. Also, Vpeak is the native unit.
			Vpeak,
			Vrms,
			Vpp
		};

		enum class AmpEnum
		{
			// Must be in this order for unit choices display index to work properly. Also, Apeak is the native unit.
			Apeak,
			Arms,
			App
		};

		enum class ChargeEnum
		{
			// Must be in this order for unit choices display index to work properly. Also, Cpeak is the native unit.
			Cpeak,
			Crms,
			Cpp
		};
		
		enum class Metric
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
			Peta
		};

		enum class UnitsEnum
		{
			None,
			RatioDB,
			RatioDBm,
			RatioPPM,
			PercentDB,
			PercentNum,
			Angle,
			Volt,
			Amp,
			Charge,
			Metric
		};


		struct GenericEnum {
		public:

			union
			{
				int None;
				RatioDBEnum ratioDB;
				RatioDBmEnum ratioDBm;
				RatioPPMEnum ratioPPM;
				PercentDBEnum percentDB;
				PercentNumEnum percentNum;
				AngleEnum angle;
				VoltEnum volt;
				AmpEnum amp;
				ChargeEnum charge;
				Metric metric;
			};

			UnitsEnum tag;

			GenericEnum() : None(0), tag(UnitsEnum::None) { };
			GenericEnum(RatioDBEnum x) : ratioDB(x), tag(UnitsEnum::RatioDB) { };
			GenericEnum(RatioDBmEnum x) : ratioDBm(x), tag(UnitsEnum::RatioDBm) { };
			GenericEnum(RatioPPMEnum x) : ratioPPM(x), tag(UnitsEnum::RatioPPM) { };
			GenericEnum(PercentDBEnum x) : percentDB(x), tag(UnitsEnum::PercentDB) { };
			GenericEnum(PercentNumEnum x) : percentNum(x), tag(UnitsEnum::PercentNum) { };
			GenericEnum(AngleEnum x) : angle(x), tag(UnitsEnum::Angle) { };
			GenericEnum(VoltEnum x) : volt(x), tag(UnitsEnum::Volt) { };
			GenericEnum(AmpEnum x) : amp(x), tag(UnitsEnum::Amp) { };
			GenericEnum(ChargeEnum x) : charge(x), tag(UnitsEnum::Charge) { };
			GenericEnum(Metric x) : metric(x), tag(UnitsEnum::Metric) { };
		};


		static constexpr std::string_view MetricPrefixes[] = { "femto", "pico", "nano", "micro", "milli", "", "kilo", "Mega", "Giga", "Tera", "Peta" };

		static constexpr std::string_view MetricPrefixMonikers[] { "f", "p", "n", "u", "m", "", "k", "M", "G", "T", "P" };

		static constexpr char MetricPrefixMonikerChars[] { 'f', 'p', 'n', 'u', 'm', '\0', 'k', 'M', 'G', 'T', 'P' };

		static constexpr int MetricExps[] { -5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5 };

		static constexpr int NoPrefixIndex = 5; // index to above where metric exp is 0

		static constexpr int Femto = -15;
		static constexpr int Pico = -12;
		static constexpr int Nano = -9;
		static constexpr int Micro = -6;
		static constexpr int Milli = -3;
		static constexpr int None = 0;
		static constexpr int Kilo = 3;
		static constexpr int Mega = 6;
		static constexpr int Giga = 9;
		static constexpr int Tera = 12;
		static constexpr int Petal = 15;

		// ////////////////////////////////////////////////////

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

		static bool IsMetricUnit(GenericEnum unit)
		{
			return unit.tag == UnitsEnum::Metric;
		}

		// ////////////////////////////////////////////////////

		static bool MetricExpInRangeStd(int mexp)
		{
			// For the standard, we only work with femto to peta
			return mexp >= -5 && mexp <= 5;
		}


		static int GetMetricExp(Metric pfx)
		{
			int i = (int)pfx;
			return MetricExps[i];
		}

		static Metric ExpToPrefix(int mexp)
		{
			return (Metric)(NoPrefixIndex + mexp);
		}

		static std::vector<std::string> GetPrefixMonikers();

		static std::vector<std::string> FormUnitMonikers(const std::string units);

#if 0
		static std::unique_ptr<std::vector<ValDesc>> FormUnitDescriptions(std::string units);
#endif

		static std::string FmtUnits(Metric pfx, const std::string units);

#if 0

		///////////////////////////////////////////////////////////////////////

		static std::string SciFmt(double v, GenericEnum selected_u, UnitChoices uc, std::string unit_desc, std::string fmt, std::string spacing = " ")
		{
			EngrVar n;

			if (uc == nullptr)
			{
				n = new EngrVar("");
			}
			else n = uc.CreateVar("");

			n.SetNum(v, selected_u);

			return n.SciFmt(fmt, unit_desc, spacing);
		}

		static std::string SciFmt(decimal v, GenericEnum selected_u, UnitChoices uc, std::string unit_desc, std::string fmt, std::string spacing = " ")
		{
			EngrVar n;

			if (uc == null)
			{
				n = new EngrVar("");
			}
			else n = uc.CreateVar("");

			n.SetNum(v, selected_u);

			return n.SciFmt(fmt, unit_desc, spacing);
		}

#endif

		double ConvertVoltageType(double old_val, VoltEnum old_u, VoltEnum new_u);
		double ConvertAmperageType(double old_val, AmpEnum old_u, AmpEnum new_u);
		double ConvertChargeType(double old_val, ChargeEnum old_u, ChargeEnum new_u);

	};

}
