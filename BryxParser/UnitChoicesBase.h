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
 * This exchange format has a nested syntax with a one to one mapping to the
 * Json syntax widely used on the web, but simplified to be easier to read and
 * write. Because of the one-to-one mapping, it's easy to translate DFX files
 * into Json files that can be processed by any software that supports Json
 * syntax.
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
#include <vector>
#include <exception>
#include "UnitPrefixes.h"

namespace RcdUtils
{

#if 0
	class UnitChoices
	{
		// ValDescs - Vals are enumerated types. Descs are strings to go with those types.
		//            This list designed for use by the units combo box.

	public:

		std::vector<ValDesc> ValDescs;

		std::vector<std::string> descriptions; // All the ways to describe the units in this set. The set used in parsing.
		int display_index;     // Index to which string to use for display
		int no_prefix_index;   // Index to the string to use when trying to indicate no metric prefix

		UnitChoices()
		{

		}

		UnitChoices(bool generate_now, int display_index_, int no_prefix_index_, const std::vector<std::string>& descriptions_)
		{
			Initialize(generate_now, display_index_, no_prefix_index_, descriptions_);
		}

		UnitChoices(int display_index_, int no_prefix_index_, const std::vector<std::string>& descriptions_)
		: UnitChoices(true, display_index_, no_prefix_index_, descriptions_)
		{

		}

		UnitChoices(const UnitChoices &s)
		: ValDescs(s.ValDescs)
		, descriptions(s.descriptions)
		, display_index(s.display_index)
		, no_prefix_index(s.no_prefix_index)
		{
			// Copy constructor

		}

		virtual ~UnitChoices() { }

		virtual UnitChoices *Clone()
		{
			return new UnitChoices(*this);
		}


		void Initialize(bool generate_now, int display_index_, int no_prefix_index_, const std::vector<std::string>& descriptions_)
		{
			display_index = display_index_;
			no_prefix_index = no_prefix_index_;

			descriptions = descriptions_;

			if (descriptions.size == 0)
			{
				throw std::exception("Must have at least one units string");
			}

			if (generate_now)
			{
				GenerateChoices();
			}
		}


		//
		// class factory for what type of EngrVar goes along with these unit choices
		//

		virtual EngrVar *CreateVar(const std::string &Name_)
		{
			return new EngrVar(Name_);
		}

		//
		// A string describing the a particular unit.
		// The default is for those units with metric prefixes
		//

		virtual std::string UnitDesc(GenericEnum u)
		{
			int i = static_cast<int>(static_cast<Units::Metric>(u));
			return ValDescs[i].Desc;
		}

		//
		// Here, we generate the all the different unit choices.
		// The default is for those units that have metric prefixes.
		//

		virtual void GenerateChoices()
		{
			auto UnitName = descriptions[display_index];

			auto uds = Units::FormUnitDescriptions(UnitName);

			// not necessary: ValDescs = new std::vector<ValDesc>;
			ValDescs.clear();

			for(ValDesc& vd : *uds)
			{
				ValDescs.push_back(std::move(vd));
			}
		}

		// /////////////////////////////////////////////////////////////
		// Now, on to parsing support
		// /////////////////////////////////////////////////////////////

		int MatchUnits(const std::string &s, int& match_len)
		{
			// Returns which index to which units matched, or -1 if no match

			int n = 0;

			for(const std::string &candy : descriptions)
			{
				// Note: we ignore case

				if (s.StartsWith(candy, StringComparison.OrdinalIgnoreCase))
				{
					match_len = candy.Length;
					return n;
				}
				++n;
			}

			return -1;
		}

		static int IsMetricPrefix(std::string s, int p, int& match_len)
		{
			// Returns powers of a thousand exponent (aka metric exp), or 0 if no match found.
			// Passes back the length of the prefix match

			// presume no match

			match_len = 0;

			int metric_exp = 0;

			std::string ss = ss.substr(p);

			if (ss != "")
			{
				// Try full prefix names first

				int i = 0;

				for(const std::string_view &candy : Units::MetricPrefixes)
				{
					if (candy != "") // We won't do that particular one
					{
						if (ss.StartsWith(candy, StringComparison.OrdinalIgnoreCase))
						{
							match_len = candy.length;
							metric_exp = Units::MetricExps[i];
							break;
						}
					}

					++i;
				}

				if (match_len == 0)
				{
					// Okay then, try the abbreviated forms (one letter). Here, case matters

					char c = ss[0];

					i = 0;

					for(char candy : Units::MetricPrefixMonikerChars)
					{
						if (candy != '\0') // Skip the null case
						{
							if (candy == c) // Case matters here
							{
								match_len = 1;
								metric_exp = Units::MetricExps[i];
								break;
							}
						}
						++i;
					}
				}

			}

			return metric_exp;
		}

		virtual bool MetricExpInRange(int me)
		{
			// We make this virtual because these exponents do not make
			// sense for all units. We use the std range by default.

			return Units::MetricExpInRangeStd(me);
		}

		virtual int ParseUnits(std::string s, int& metric_exp, int& matching_units_index)
		{
			// See if suffix s matches the units we are expecting.
			// We allow an optional metric prefix.
			// We don't care about case on the units, but we do care about case on the
			// metric prefix. Eg., p = pico, P = peta, 27 orders of magnitude difference!

			// NOTE: This parse logic is the way it is to support units entry in our unit var field entries.
			// The logic works as follows: If a metric prefix is mentioned in the string, then the metric
			// exp parameter is updated. If no prefix is mentioned, then the exponent is set to zero, 
			// but if and only if a unit is also given. (This is the only way we can denote our intentions
			// to have a metric exponent of 0.) If no unit is given, and no metric prefix is mentioned,
			// then this is a signal to leave the metric exp alone.
			// @@ What do we do if there's no match on the units, but there are leftover characters?
			// Do we not update the metric exp? I'm thinking, correct. We don't up update the exponent,
			// for what we have is clearly an error. Best to leave dead dogs lie.

			// Yeah, this is all tricky and subtle, but makes sense at the user level.

			matching_units_index = -1; // Default to no units found.

			int p = 0;

			if (s.length > 0)
			{
				// Record any metric prefix mentioned

				int prefix_len = 0;

				int new_metric_exp = IsMetricPrefix(s, p, prefix_len);

				if (new_metric_exp != 0)
				{
					// Skip said prefix
					p += prefix_len;
				}

				// Now, try to match the rest of the string to one of our possible unit strings

				std::string rest = s.substr(p);

				if (rest == "")
				{
					// We have no units, so we wish to update the metric exp, but if and
					// only if a new metric prefix was mentioned.
					// @@ Hmm. We should *always* succeed here.

					if (new_metric_exp != 0)
					{
						metric_exp = new_metric_exp;
					}
				}
				else
				{
					int units_len = 0;

					int r = MatchUnits(rest, units_len);

					if (r != -1)
					{
						matching_units_index = r;

						// Units match found, so skip over the characters

						//p += rest.Length; // @@ Or units length?
						p += units_len;

						// Update metric exponent, if one was indicated, else set it to zero

						if (new_metric_exp != 0)
						{
							metric_exp = new_metric_exp; // we only return change if there was in fact a change
						}
						else metric_exp = 0;
					}
					else
					{
						// Match not found. So if we had a metric prefix, back up and try to match just units

						if (new_metric_exp != 0)
						{
							p -= prefix_len; // back up

							// Let's do a rematch on the whole kabosh

							rest = s.substr(p);

							r = MatchUnits(rest, units_len);

							if (r != -1)
							{
								matching_units_index = r;

								// Match found on the units. So we didn't have a metric prefix after all.
								// Thus, set the exp to zero per logic described at the beginning of this function.

								metric_exp = 0;

								// Skip over the units characters
								// p += rest.Length; // @@ Or units length (?)

								p += units_len;
							}
							else
							{
								// No units match found. We have an error condition.               
								// So we just keep the cursor where it is, and we don't update the exponent.
							}
						}
						else
						{
							// we'uz finished. Nothing we can do. We have's us an error.
						}
					}
				}
			}
			else
			{
				// We have empty string. If we had a metric exponent and it's not in range, then we need to reset it.
				// (Such exponents don't get special treatment. They aren't "sticky" the way the in-range ones are.)

				bool exp_in_range = MetricExpInRange(metric_exp);

				if (!exp_in_range)
				{
					// means we had exponent "with" the mantissa (a non-metric exponent). So we want to completely drop it.
					metric_exp = 0;
				}
			}

			return p;
		}

	}

#endif

}; // Of namespace
