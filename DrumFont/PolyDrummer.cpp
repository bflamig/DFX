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

#include "PolyDrummer.h"

namespace dfx
{
    // Just a starter kludge

	static unsigned char pianoKeyToDrumMap[128] =
	{ 0,0,0,0,0,0,0,0,		// 0-7
	  0,0,0,0,0,0,0,0,		// 8-15
	  0,0,0,0,0,0,0,0,		// 16-23
	  0,0,0,0,0,0,0,0,		// 24-31
	  0,0,0,0,1,0,2,0,		// 32-39
	  2,3,6,3,6,4,7,4,		// 40-47
	  5,8,5,0,0,0,10,0,		// 48-55
	  9,0,0,0,1,2,3,4,		// 56-63
	  5,6,7,8,9,10,11,12,	// 64-71
	  13,14,0,0,0,0,0,0,	// 72-79
	  0,0,0,0,0,0,0,0,		// 80-87
	  0,0,0,0,0,0,0,0,		// 88-95
	  0,0,0,0,0,0,0,0,		// 96-103
	  0,0,0,0,0,0,0,0,		// 104-111
	  0,0,0,0,0,0,0,0,		// 112-119
	  0,0,0,0,0,0,0,0       // 120-127
	};


	PolyDrummer::PolyDrummer(int polyPhony)
	: polyTable(polyPhony)
	, interrupt_same_note(false)  // @@ don't use true till fixed, if ever
	{
	}

	PolyDrummer::~PolyDrummer()
	{
	}

	void PolyDrummer::LoadKit(std::shared_ptr<DrumKit> &kit_)
	{
		drumKit = kit_;
	}

	bool PolyDrummer::HasSoundsToPlay()
	{
		int i = polyTable.aHead;
		return i != -1;
	}

	void PolyDrummer::noteOnDirect(int noteNumber, double amplitude)
	{
#ifdef DFX_DEBUG
		if (amplitude < 0.0 || amplitude > 1.0)
		{
			throw std::exception("PolyDrum::noteOn: amplitude parameter is out of bounds");
		}
#endif

		int slot = -1;

		if (interrupt_same_note)
		{
			// In this scheme we search for the note already being active.
			// If so, we merely start the active wave over by resetting it

			auto& e = polyTable.elems;
			slot = polyTable.aHead;

			while (slot != -1)
			{
				if (e[slot].soundNumber == noteNumber)
				{
					e[slot].wave.Reset();
					break;
				}

				slot = e[slot].older;
			}
		}

		if (slot == -1) // Not reusing existing same note wave
		{
			// Look first for an unused wave or preempt the
			// oldest if already at maximum polyphony.

			if (polyTable.IsFull())
			{
				// Turn off oldest since we're going to be using its slot. 
				// @@ Actually, don't need to do anything here.
			}

			// grab a slot to use and place on active list
			// if table is full, we reuse the oldest slot

			slot = polyTable.ActivateSlot(noteNumber);

			// Point to the proper sound wave to use

			auto& e = polyTable.elems[slot];

			int mapped_note = pianoKeyToDrumMap[noteNumber]; // temporary kludge
			auto& drum = drumKit->noteMap[mapped_note];
			if (drum)
			{
				auto& mw = drum->ChooseWave(amplitude);
				e.wave.AliasSamples(mw);
			}
			else return; // No drum mapped, so do nothing
		}

		auto& e = polyTable.elems[slot];
		e.gain = amplitude; // experiment

#if 0
		e.filter.setPole(0.999 - (amplitude * 0.6));
		e.filter.setGain(amplitude);
#endif

	}

#if 0
	void PolyDrummer::noteOn(double instrument, double amplitude)
	{
		// Yes, this is tres kludgey.
		int noteNumber = (int)((12 * log(instrument / 220.0) / log(2.0)) + 57.01);

		noteOnDirect(noteNumber, amplitude);
	}
#endif

	void PolyDrummer::noteOff(double amplitude)
	{
		// Set all sounding wave filter gains low.

		int i = polyTable.aHead;

		while (i != -1)
		{
			auto& e = polyTable.elems[i];
			// @@ TODO: DO THIS! e.filter.setGain(amplitude * 0.01);
			i = e.older;
		}
	}


	StereoFrame<double> PolyDrummer::StereoTick()
	{
		double left = 0.0;
		double right = 0.0;

		int i = polyTable.aHead;

		while (i != -1)
		{
			int nxt = polyTable.elems[i].older;

			if (polyTable.elems[i].wave.IsFinished())
			{
				polyTable.Deactivate(i);
			}
			else
			{
				auto& e = polyTable.elems[i];
				auto rv = e.wave.StereoTick();
				left += rv.left;   // @@ TODO: l * e.gain?
				right += rv.right;  // @@ TODO: r * e.gain?
			}

			i = nxt;
		}

		return { left, right };
	}

}