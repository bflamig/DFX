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

#include <vector>
#include <ostream>
#include "MemWave.h"
//#include "OnePole.h"

namespace dfx
{
	class PolyTableElem {
	public:

		MemWave wave;      // resident wave storage
		//OnePole filter;  // optional filter 

		double gain;

		int soundNumber;  // Used if wanting to reset active wave of same note as new one.
		int younger;      // (younger) for doubly linked active list
		int older;        // (older) for doubly linked active list, and singly linked inactive list

	public:

		PolyTableElem();
	};


	class PolyTable {
	public:
		std::vector<PolyTableElem> elems;
		int aHead;    // to first active slot
		int iHead;    // to first inactive slot
		int aOldest;  // to oldest (last) active slot
	public:
		PolyTable(int nsoundings);
		virtual ~PolyTable();
	public:
		bool IsFull() const { return iHead == -1; }
		int ActivateSlot(int noteNumber);
		void Deactivate(int slot);
	protected:
		void MakeYoungest(int slot); // moves to first of the actives
	public:
		void DumpActive(std::ostream& s);
		void DumpInactive(std::ostream& s);
	};

} // end of namespace