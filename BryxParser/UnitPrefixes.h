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
#include "ValDesc.h"

#if 0

namespace RcdUtils
{

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
			Vpeak, // Must be in this order for unit choices display index to work properly. Also, Vpeak is the native units
			Vrms,
			Vpp
		};

		enum class AmpEnum
		{
			Apeak, // Must be in this order for unit choices display index to work properly. Also, Apeak is the native units
			Arms,
			App
		};

		enum class ChargeEnum
		{
			Cpeak, // Must be in this order for unit choices display index to work properly. Also, Cpeak is the native units
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

		static double peta(double v)
		{
			return v * 1.0e+15;
		}

		static double tera(double v)
		{
			return v * 1.0e+12;
		}

		static double giga(double v)
		{
			return v * 1.0e+9;
		}

		static double mega(double v)
		{
			return v * 1.0e+6;
		}

		static double kilo(double v)
		{
			return v * 1.0e+3;
		}

		static double none(double v)
		{
			return v;
		}

		static double milli(double v)
		{
			return v * 1.0e-3;
		}

		static double micro(double v)
		{
			return v * 1.0e-6;
		}

		static double nano(double v)
		{
			return v * 1.0e-9;
		}

		static double pico(double v)
		{
			return v * 1.0e-12;
		}

		static double femto(double v) 
		{
			return v * 1.0e-15;
		}

#if 0
		static bool IsMetricUnit(Enum unit)
		{
			Type a = typeof(Units.Metric);
			Type b = unit.GetType();

			return a.Equals(b);
		}
#endif

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

		static std::vector<std::string> GetPrefixMonikers()
		{
			std::vector<std::string> list;

			for(auto &sv : MetricPrefixMonikers)
			{
				list.push_back(move(std::string(sv)));
			}

			return list;
		}

		static std::vector<std::string> FormUnitMonikers(std::string units)
		{
			std::vector<std::string> list;

			for(auto &sv : MetricPrefixMonikers)
			{
				std::string s(sv);
				list.push_back(move(s + units));
			}

			return list;
		}

#if 1

		static std::unique_ptr<std::vector<ValDesc>> FormUnitDescriptions(std::string units)
		{
			auto list = std::make_unique<std::vector<ValDesc>>();

			for (auto pfx : Units::MetricPrefixMonikers)
			{
				list.push_back(ValDesc(e, FmtUnits(pfx, units)));
			}
		}

#endif

		static std::string FmtUnits(Metric pfx, std::string units)
		{
			if (pfx == Metric::None)
			{
				return units;
			}
			else return std::string(MetricPrefixMonikers[(int)pfx]) + units;
		}

#if 1

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

		// Support for converting betweenxt the common voltage types, (peak, peak-to-peak, rms)

		static double ConvertVoltageType(double old_val, Units::VoltEnum old_u, Units::VoltEnum new_u)
		{
			double new_val = old_val;

			constexpr double sqrt2 = std::sqrt(2.0);

			if (old_u == Units::VoltEnum::Vpeak)
			{
				switch (new_u)
				{
					case Units::VoltEnum::Vpp:
					new_val = 2.0 * old_val;
					break;
					case Units::VoltEnum::Vrms:
					new_val = old_val / sqrt2;
					break;
				}
			}
			else if (old_u == Units::VoltEnum::Vpp)
			{
				switch (new_u)
				{
					case Units::VoltEnum::Vpeak:
					new_val = old_val / 2.0;
					break;
					case Units::VoltEnum::Vrms:
					new_val = old_val / (2.0 * sqrt2);
					break;
				}

			}
			else // old_vu == Units.VoltEnum.Vrms
			{
				switch (new_u)
				{
					case Units::VoltEnum::Vpeak:
					new_val = old_val * sqrt2;

					break;
					case Units::VoltEnum::Vpp:
					new_val = old_val * 2.0 * sqrt2;
					break;

				}
			}

			return new_val;
		}


		// Support for converting betweenxt the common amperage types, (peak, peak-to-peak, rms)

		static double ConvertAmperageType(double old_val, Units::AmpEnum old_u, Units::AmpEnum new_u)
		{
			double new_val = old_val;

			constexpr double sqrt2 = std::sqrt(2.0);

			if (old_u == Units::AmpEnum::Apeak)
			{
				switch (new_u)
				{
					case Units::AmpEnum::App:
					new_val = 2.0 * old_val;
					break;
					case Units::AmpEnum::Arms:
					new_val = old_val / sqrt2;
					break;
				}
			}
			else if (old_u == Units::AmpEnum::App)
			{
				switch (new_u)
				{
					case Units::AmpEnum::Apeak:
					new_val = old_val / 2.0;
					break;
					case Units::AmpEnum::Arms:
					new_val = old_val / (2.0 * sqrt2);
					break;
				}

			}
			else // old_u == Units.AmpEnum.Arms
			{
				switch (new_u)
				{
					case Units::AmpEnum::Apeak:
					new_val = old_val * sqrt2;

					break;
					case Units::AmpEnum::App:
					new_val = old_val * 2.0 * sqrt2;
					break;

				}

			}

			return new_val;
		}

		// Support for converting betweenxt the common charge types, (peak, peak-to-peak, rms)

		static double ConvertChargeType(double old_val, Units::ChargeEnum old_u, Units::ChargeEnum new_u)
		{
			double new_val = old_val;

			constexpr double sqrt2 = std::sqrt(2.0);

			if (old_u == Units::ChargeEnum::Cpeak)
			{
				switch (new_u)
				{
					case Units::ChargeEnum::Cpp:
					new_val = 2.0 * old_val;
					break;
					case Units::ChargeEnum::Crms:
					new_val = old_val / sqrt2;
					break;
				}
			}
			else if (old_u == Units::ChargeEnum::Cpp)
			{
				switch (new_u)
				{
					case Units::ChargeEnum::Cpeak:
					new_val = old_val / 2.0;
					break;
					case Units::ChargeEnum::Crms:
					new_val = old_val / (2.0 * sqrt2);
					break;
				}

			}
			else // old_u == Units.AmpEnum.Arms
			{
				switch (new_u)
				{
					case Units::ChargeEnum::Cpeak:
					new_val = old_val * sqrt2;

					break;
					case Units::ChargeEnum::Cpp:
					new_val = old_val * 2.0 * sqrt2;
					break;

				}

			}

			return new_val;
		}

#endif

	};

}


#endif