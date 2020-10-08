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

#include <filesystem>
#include "MultiLayeredDrum.h"

class DrumKit {
public:
	std::filesystem::path cumulativePath; // For ease of recursing down
	std::filesystem::path basePath;       // Usually the path of the sound font file
	std::filesystem::path kitPath;        // Relative to sound font location
	std::string name;

	std::vector<std::shared_ptr<MultiLayeredDrum>> drums;
	std::vector<std::shared_ptr<MultiLayeredDrum>> noteMap;

public:

	DrumKit();
	DrumKit(const std::string& name_, const std::string& kitPath_);
	DrumKit(const DrumKit& other);
	DrumKit(DrumKit&& other) noexcept;

	virtual ~DrumKit();

public:

	void ClearNotes();
	void FinishPaths(std::filesystem::path& soundFontPath_);
	void BuildNoteMap();

};

