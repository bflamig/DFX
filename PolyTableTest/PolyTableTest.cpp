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

#include <iostream>
#include "PolyTable.h"

using namespace bryx;

int polytest1()
{
	PolyTable pt(3);

	pt.DumpActive(std::cout);
	pt.DumpInactive(std::cout);

	int note;

	note = 87;
	pt.ActivateSlot(note);

	std::cout << "--- After activating slot with note " << note << " ---" << "\n\n";
	pt.DumpActive(std::cout);
	pt.DumpInactive(std::cout);

	note = 36;
	pt.ActivateSlot(note);

	std::cout << "--- After activating slot with note " << note << " ---" << "\n\n";
	pt.DumpActive(std::cout);
	pt.DumpInactive(std::cout);

	note = 55;
	pt.ActivateSlot(note);

	std::cout << "--- After activating slot with note " << note << " ---" << "\n\n";
	pt.DumpActive(std::cout);
	pt.DumpInactive(std::cout);

	//pt.Deactivate(0);
	//pt.Deactivate(1);
	//pt.Deactivate(2);

	note = 75;
	pt.ActivateSlot(note); // Full, so need to reuse oldest

	std::cout << "--- After activating slot with note " << note << " in full table ---" << "\n\n";
	pt.DumpActive(std::cout);
	pt.DumpInactive(std::cout);

	pt.Deactivate(1);

	std::cout << "--- After deactivating slot 1 ---" << "\n\n";
	pt.DumpActive(std::cout);
	pt.DumpInactive(std::cout);

	pt.Deactivate(2);

	std::cout << "--- After deactivating slot 2 ---" << "\n\n";
	pt.DumpActive(std::cout);
	pt.DumpInactive(std::cout);

	pt.Deactivate(0);

	std::cout << "--- After deactivating slot 0 ---" << "\n\n";
	pt.DumpActive(std::cout);
	pt.DumpInactive(std::cout);

	return 0;
}


int main()
{
	polytest1();
}

