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

#include <algorithm>
#include "RobinMgr.h"

// ////////////////////////////////////////////////////

Robin::Robin(std::string fileName_, double peak_, double rms_, size_t offset_)
: fullPath()
, fileName(fileName_)
, peak(peak_)
, rms(rms_)
, offset(offset_)
{

}

Robin::Robin(const Robin& other)
: fullPath(other.fullPath)
, fileName(other.fileName)
, peak(other.peak)
, rms(other.rms)
, offset(other.offset)
{

}

Robin::Robin(Robin&& other) noexcept
: fullPath(std::move(other.fullPath))
, fileName(std::move(other.fileName))
, peak(other.peak)
, rms(other.rms)
, offset(other.offset)
{
}


Robin& Robin::operator=(const Robin& other)
{
	if (this != &other)
	{
		fullPath = other.fullPath;
		fileName = other.fileName;
		peak = other.peak;
		rms = other.rms;
		offset = other.offset;
	}

	return *this;
}

Robin& Robin::operator=(Robin&& other) noexcept
{
	if (this != &other)
	{
		fullPath = std::move(other.fullPath);
		fileName = std::move(other.fileName);
		peak = other.peak;
		rms = other.rms;
		offset = other.offset;
	}

	return *this;
}

void Robin::FinishPaths(std::filesystem::path& cumulativePath_)
{
	fullPath = cumulativePath_;
	fullPath /= fileName;
	fullPath = fullPath.generic_string();
}



// ////////////////////////////////////////////////////


RobinMgr::RobinMgr()
: robins{}
, lastRobinChosen(0)
{
}

RobinMgr::RobinMgr(const RobinMgr& other)
{
	robins = other.robins;
	lastRobinChosen = other.lastRobinChosen;
}

RobinMgr::RobinMgr(RobinMgr&& other) noexcept
{
	robins = std::move(other.robins);
	lastRobinChosen = other.lastRobinChosen;
	other.lastRobinChosen = 0;
}

RobinMgr::~RobinMgr()
{
}

RobinMgr& RobinMgr::operator=(const RobinMgr& other)
{
	if (this != &other)
	{
		robins = other.robins;
		lastRobinChosen = other.lastRobinChosen;
	}

	return *this;
}

RobinMgr& RobinMgr::operator=(RobinMgr&& other) noexcept
{
	if (this != &other)
	{
		robins = std::move(other.robins);
		lastRobinChosen = other.lastRobinChosen;
		other.lastRobinChosen = 0;
	}

	return *this;
}

void RobinMgr::FinishPaths(std::filesystem::path& cumulativePath_)
{
	for (auto& robin : robins)
	{
		robin.FinishPaths(cumulativePath_);
	}
}




#if 0

void Robins::LoadWaves(std::string pathToWaves, bool is_directory, FileEncoding encoding)
{
	try
	{
		int nrobins = 0;

		if (is_directory)
		{
			fs::directory_iterator di(pathToWaves);

			for (auto d : di)
			{
				nrobins++;
			}
		}
		else nrobins = 1;

		waves.resize(nrobins);

		// We'll initialize the last chosen round robin index
		// to be the last variation. That way, next time around,
		// the first will be chosen.

		lastRobinChosen = nrobins - 1;

		if (is_directory)
		{
			// apparently you can't reuse an iterator? 

			fs::directory_iterator effme(pathToWaves);

			int i = 0;

			for (auto d : effme)
			{
				auto ocean = d.path().string(); // d.path().c_str();
				waves[i].Load(ocean, encoding);

				// Store just the file name for convenience. We do it
				// here, where we still have unicode stuff around 
				// that we can use.

				auto effyname = d.path().filename();
				waves[i].fileName = effyname.string(); //  convert.to_bytes(effyname);

				i++;
			}
		}
		else
		{
			// We have just a single variation
			waves[0].Load(pathToWaves, encoding);
			// Wrap full path with a path helper so we can get just the file name.
			fs::path p(pathToWaves);
			waves[0].fileName = p.filename().string();
		}
	}
	catch (...) // std::exception &e)
	{
		Stk::oStream_ << "Robins::LoadWaves(): Error loading wave files in directory:\n";
		Stk::oStream_ << "File: " << pathToWaves;
		Stk::handleError(StkError::FILE_ERROR);
	}
}

#endif

// Using simple round robin for now

Robin& RobinMgr::ChooseRobin()
{
	++lastRobinChosen;

	if (lastRobinChosen >= robins.size())
	{
		lastRobinChosen = 0;
	}

	return robins[lastRobinChosen];
}
