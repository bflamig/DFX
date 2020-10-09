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

#include "PolyTable.h"
#include "DrumKit.h"

namespace dfx
{
	struct Frame
	{
		double left;
		double right;
	};

	constexpr int DRUM_POLYPHONY = 16;

	class PolyDrummer {
	public:

		PolyTable polyTable;
		DrumKit drumKit;

		bool interrupt_same_note; // If true, only one playback of each note active at a time.

	public:

		PolyDrummer(int polyPhony = DRUM_POLYPHONY);

		virtual ~PolyDrummer();

		void LoadKit();

		void SetInterruptSameNoteScheme(bool reuse_flag)
		{
			interrupt_same_note = reuse_flag;
		}

		//! Start a note with the given drum type and amplitude.

		void noteOnDirect(int number, double amplitude);

		//! Start a note with the given drum type and amplitude.
		//void noteOn(double instrument, double amplitude);

		//! Stop a note with the given amplitude (speed of decay).
		void noteOff(double amplitude);

		//! Compute and return one output sample.
		//double tick(unsigned int channel = 0);

		std::pair<double, double> StereoTick();

		//! Fill a channel of the Frame object with computed outputs.
		//Frame& tick(Frame& frame, unsigned int channel = 0);


	};


} // end of namespace