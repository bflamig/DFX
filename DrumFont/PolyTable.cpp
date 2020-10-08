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

namespace bryx
{

	// //////////////////////////////////////////////////////////

	PolyTableElem::PolyTableElem()
		: wave()
		//, filter()
		, gain(0)
		, soundNumber(0)
		, younger(-1)
		, older(-1)
	{

	}


	// ///////////////////////////////////////////////////////////

	PolyTable::PolyTable(int nsoundings)
		: elems(nsoundings)
	{
		// Set up inactive linked list to take up entire table.

		for (int i = 0; i < nsoundings; i++)
		{
			elems[i].younger = -1;  // Only for active list, which starts out empty
			elems[i].older = i + 1;
			elems[i].soundNumber = -1;
		}

		// Fixup last inactive older pointer

		elems[nsoundings - 1].older = -1;

		iHead = 0; // Head of inactive list

		// Set up active linked list. It's completely empty.

		aHead = -1;     // Head of active list (empty)
		aOldest = -1;   // No oldest slot yet
	}

	PolyTable::~PolyTable()
	{
	}

	int PolyTable::ActivateSlot(int noteNumber)
	{
		// Uses the first inactive slot and places it on the active list.
		// If active list is full, we reuse the oldest active slot.
		// Returns the slot used.

		int slot;

		if (iHead == -1)
		{
			// We are full, so we'll reuse the oldest active slot
			// and make second oldest slot the new oldest
			slot = aOldest;
			aOldest = elems[slot].younger;
			elems[aOldest].older = -1;
			MakeYoungest(slot);
		}
		else
		{
			// Our table isn't full, so ...
			// So grab head from inactive list to use for our new active slot

			slot = iHead;

			if (aHead == -1)
			{
				// No active slots yet, so there will be a new oldest active
				aOldest = slot;
			}

			// Remove from inactive list by simply advancing the inactive head

			iHead = elems[slot].older;

			// Place slot on the active list. We make it the head of that list (youngest).

			MakeYoungest(slot);
		}

		elems[slot].soundNumber = noteNumber;

		return slot;
	}

	void PolyTable::MakeYoungest(int slot)
	{
		// Low level funtion ONLY. ASSUMES slot is *not* on the inactive list,
		// since we don't do any surgery on that list here.

		// add to head of active list (so we become youngest)

		if (aHead != -1)
		{
			elems[aHead].younger = slot;
		}

		elems[slot].younger = -1;
		elems[slot].older = aHead;
		aHead = slot;
	}

	void PolyTable::Deactivate(int slot)
	{
		// We presume slot is somewhere on the active list.
		// Place slot on the inactive list.

		// But first, if this slot is the oldest, then bump that
		// indicator to second oldest.

		if (slot == aOldest)
		{
			aOldest = elems[aOldest].younger;
		}

		// Okay, onwards

		if (elems[slot].younger == -1)
		{
			// At head of active list.
			// Remove from active list by simply advancing active head.

			aHead = elems[slot].older;
			if (aHead != -1)
			{
				elems[aHead].younger = -1;
			}
		}
		else
		{
			// We're somewhere after first slot of active list.
			// Remove from active list by splicing it out.
			int p = elems[slot].younger;
			int n = elems[slot].older;

			elems[p].older = n;

			if (n != -1)
			{
				elems[n].younger = p;
			}
		}

		// add to head of inactive list

		elems[slot].older = iHead;
		iHead = slot;

		// To help remove debugging confusion:

		elems[slot].younger = -1; // only used when on active list anyway
	}

	void PolyTable::DumpActive(std::ostream& s)
	{
		s << "Active List from youngest to oldest:\n";

		if (aHead == -1)
		{
			s << "  <empty>\n";
		}
		else
		{
			int x = aHead;
			while (x != -1)
			{
				const auto& e = elems[x];
				s << "  slot " << x << ": key = " << e.soundNumber << ", younger = " << e.younger << ", older = " << e.older << "\n";
				x = e.older;
			}
		}

		s << '\n';
	}

	void PolyTable::DumpInactive(std::ostream& s)
	{
		s << "Inactive List from youngest to oldest:\n";

		if (iHead == -1)
		{
			s << "  <empty>\n";
		}
		else
		{
			int x = iHead;
			while (x != -1)
			{
				const auto& e = elems[x];
				s << "  slot " << x << ": key = " << e.soundNumber << ", older = " << e.older << "\n";
				x = e.older;
			}
		}

		s << '\n';
	}

} // end of namespace