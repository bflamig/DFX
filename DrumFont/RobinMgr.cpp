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

namespace dfx
{
	// ////////////////////////////////////////////////////

	Robin::Robin(std::string fileName_, double peak_, double rms_, unsigned start_frame_, unsigned end_frame_)
	: wave()
	, fullPath()
	, fileName(fileName_)
	, peak(peak_)
	, rms(rms_)
	, start_frame(start_frame_)
	, end_frame(end_frame_)
	{

	}

	Robin::Robin(const Robin& other)
	: wave(other.wave)
	, fullPath(other.fullPath)
	, fileName(other.fileName)
	, peak(other.peak)
	, rms(other.rms)
	, start_frame(other.start_frame)
	, end_frame(other.end_frame)
	{

	}

	Robin::Robin(Robin&& other) noexcept
	: wave(std::move(other.wave))
	, fullPath(std::move(other.fullPath))
	, fileName(std::move(other.fileName))
	, peak(other.peak)
	, rms(other.rms)
	, start_frame(other.start_frame)
	, end_frame(other.end_frame)
	{
		// Just keeping move pedantics :)
		other.peak = 0;
		other.rms = 0;
		other.start_frame = 0;
		other.end_frame = 0;
	}


	Robin& Robin::operator=(const Robin& other)
	{
		if (this != &other)
		{
			wave = other.wave;
			fullPath = other.fullPath;
			fileName = other.fileName;
			peak = other.peak;
			rms = other.rms;
			start_frame = other.start_frame;
			end_frame = other.end_frame;
		}

		return *this;
	}

	Robin& Robin::operator=(Robin&& other) noexcept
	{
		if (this != &other)
		{
			wave = std::move(other.wave);
			fullPath = std::move(other.fullPath);
			fileName = std::move(other.fileName);
			peak = other.peak;
			rms = other.rms;
			start_frame = other.start_frame;
			end_frame = other.end_frame;

			// Just keeping move pedantics :)
			other.peak = 0;
			other.rms = 0;
			other.start_frame = 0;
			other.end_frame = 0;
		}

		return *this;
	}

	void Robin::FinishPaths(std::filesystem::path& cumulativePath_)
	{
		fullPath = cumulativePath_;
		fullPath /= fileName;
		fullPath = fullPath.generic_string();
	}

	bool Robin::LoadWave(std::ostream &serr)
	{
		// NOTE: My jungle drums already have their dynamics tuned
		// just right. And the robin files are already scaled with
		// that in mind. So we don't want to do anything else here.
		// @@ TODO: We need a sound font option that says leave 
		// this as is. Or, we always do this peak scaling, but 
		// we need knee curves instead dyn range curves for
		// this drum, and then just replicate what's in the drum (?)
		// Subtle effects on wave choices though, aren't there?
		// We really have two velocity curves per instrument. One
		// helps select the velocity layer (and depending on how it
		// was recorded, also it's "volume"), the other, the actual
		// amplitude played. The 1 / peak below normalizes out the
		// "volume" the wave was recorded at, but we still have
		// its seleciton of layer intact, implied in the velocity
		// code sent.

		bool au_naturale = true;  // @@ for now!
		double scale_factor_code = au_naturale ? 1.0 : 1.0 / peak;

		bool b = wave.Load(fullPath, start_frame, end_frame, scale_factor_code);
		if (!b)
		{
			serr << "Error loading file: " << fullPath << std::endl;
		}
		return b;
	}

	// ////////////////////////////////////////////////////


	RobinMgr::RobinMgr()
	: robins{}
	, lastRobinChosen{ 0 }
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

	int RobinMgr::LoadWaves(std::ostream &serr)
	{
		int errcnt = 0;

		for (auto& r : robins)
		{
			bool b = r.LoadWave(serr);
			if (!b)
			{
				++errcnt;
			}
		}

		return errcnt;
	}

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

	MemWave& RobinMgr::ChooseWave()
	{
		auto& robin = ChooseRobin();
		return robin.wave;
	}

} // end of namespace