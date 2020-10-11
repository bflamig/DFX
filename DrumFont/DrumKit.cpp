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
#include "DrumKit.h"

namespace dfx
{

	DrumKit::DrumKit()
	: noteMap{ 128 }
	{
		//std::cout << "DrumKit default ctor called" << std::endl;
	}

	DrumKit::DrumKit(const std::string& name_, const std::string& kitPath_)
	: cumulativePath()
	, basePath()
	, kitPath(kitPath_)
	, name(name_)
	, drums()
	, noteMap{ 128 }
	{
		//std::cout << "DrumKit ctor called" << std::endl;
	}

	DrumKit::DrumKit(const DrumKit& other)
	: cumulativePath(other.cumulativePath)
	, basePath(other.basePath)
	, kitPath(other.kitPath)
	, name(other.name)
	, drums(other.drums)
	, noteMap{ 128 }
	{
		//std::cout << "Drumkit copy ctor called" << std::endl;
	}

	DrumKit::DrumKit(DrumKit&& other) noexcept
	: cumulativePath(std::move(other.cumulativePath))
	, basePath(std::move(other.basePath))
	, kitPath(std::move(other.kitPath))
	, name(std::move(other.name))
	, drums(std::move(other.drums))
	, noteMap(std::move(other.noteMap))
	{
		//std::cout << "DrumKit mtor called" << std::endl;
	}

	DrumKit::~DrumKit()
	{
		//std::cout << "DrumKit dtor called" << std::endl;
	}

	void DrumKit::operator=(const DrumKit& other)
	{
		// Copy assignment

		if (this != &other)
		{
			cumulativePath = other.cumulativePath;
			basePath = other.basePath;
			kitPath = other.kitPath;
			name = other.name;
			drums = other.drums;
			noteMap = other.noteMap;
		}
	}

	void DrumKit::operator=(DrumKit&& other) noexcept
	{
		// Move assignment

		if (this != &other)
		{
			cumulativePath = std::move(other.cumulativePath);
			basePath = std::move(other.basePath);
			kitPath = std::move(other.kitPath);
			name = std::move(other.name);
			drums = std::move(other.drums);
			noteMap = std::move(other.noteMap);
		}
	}


	void DrumKit::ClearNotes()
	{
		for (auto& n : noteMap)
		{
			n = nullptr;
		}
	}

	void DrumKit::FinishPaths(std::filesystem::path& soundFontPath_)
	{
		basePath = soundFontPath_;
		basePath.remove_filename();

		cumulativePath = basePath;
		cumulativePath /= kitPath;
		cumulativePath = cumulativePath.generic_string(); // Sigh! THey just had to do it wrong!

		for (auto& d : drums)
		{
			d->cumulativePath = cumulativePath;
			d->cumulativePath /= d->drumPath;
			d->cumulativePath = d->cumulativePath.generic_string();

			d->SortLayers();

			for (auto &layer : d->velocityLayers)
			{
				layer.FinishPaths(d->cumulativePath);
			}
		}
	}

	void DrumKit::BuildNoteMap()
	{
		ClearNotes();

		for (auto& d : drums)
		{
			auto m = d->midiNote;

			if (noteMap[m] == nullptr)
			{
				noteMap[m] = d;
			}
			else
			{
				// @@ Error! Note conflict! Perhaps catch this at verify parse time ?
			}
		}
	}

} // end of namespace