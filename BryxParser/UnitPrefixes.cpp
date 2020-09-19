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

#include "UnitPrefixes.h"

namespace bryx
{
	static const double sqrt2 = std::sqrt(2);
	static const double one_over_sqrt2 = 1.0 / std::sqrt(2);
	static const double one_over_two_times_sqrt2 = 1.0 / (2.0 * std::sqrt(2));


	std::vector<std::string> Units::GetPrefixMonikers()
	{
		std::vector<std::string> list;

		for (auto& sv : MetricPrefixMonikers)
		{
			list.push_back(move(std::string(sv)));
		}

		return list;
	}

	std::vector<std::string> Units::FormUnitMonikers(const std::string units)
	{
		std::vector<std::string> list;

		for (auto& sv : MetricPrefixMonikers)
		{
			std::string s(sv);
			list.push_back(move(s + units));
		}

		return list;
	}

#if 0
	std::unique_ptr<std::vector<ValDesc>> Units::FormUnitDescriptions(std::string units)
	{
		auto list = std::make_unique<std::vector<ValDesc>>();

		for (auto pfx : Units::MetricPrefixMonikers)
		{
			list.push_back(ValDesc(e, FmtUnits(pfx, units)));
		}
	}
#endif

	std::string Units::FmtUnits(Metric pfx, const std::string units)
	{
		if (pfx == Metric::None)
		{
			return units;
		}
		else return std::string(MetricPrefixMonikers[(int)pfx]) + units;
	}

	// Support for converting between the common voltage types, (peak, peak-to-peak, rms)

	double Units::ConvertVoltageType(double old_val, VoltEnum old_u, VoltEnum new_u)
	{
		double new_val = old_val;

		if (old_u == Units::VoltEnum::Vpeak)
		{
			switch (new_u)
			{
				case Units::VoltEnum::Vpp:
				new_val = 2.0 * old_val;
				break;
				case Units::VoltEnum::Vrms:
				new_val = old_val * one_over_sqrt2;
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
				new_val = old_val * one_over_two_times_sqrt2;
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


	// Support for converting between the common amperage types, (peak, peak-to-peak, rms)

	double Units::ConvertAmperageType(double old_val, AmpEnum old_u, AmpEnum new_u)
	{
		double new_val = old_val;

		if (old_u == Units::AmpEnum::Apeak)
		{
			switch (new_u)
			{
				case Units::AmpEnum::App:
				new_val = 2.0 * old_val;
				break;
				case Units::AmpEnum::Arms:
				new_val = old_val * one_over_sqrt2;
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
				new_val = old_val * one_over_two_times_sqrt2;
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

	// Support for converting between the common charge types, (peak, peak-to-peak, rms)

	double Units::ConvertChargeType(double old_val, ChargeEnum old_u, ChargeEnum new_u)
	{
		double new_val = old_val;

		if (old_u == Units::ChargeEnum::Cpeak)
		{
			switch (new_u)
			{
				case Units::ChargeEnum::Cpp:
				new_val = 2.0 * old_val;
				break;
				case Units::ChargeEnum::Crms:
				new_val = old_val * one_over_sqrt2;
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
				new_val = old_val * one_over_two_times_sqrt2;
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

} // of namespace