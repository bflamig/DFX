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
#include <string>
#include <filesystem>
#include "MemWave.h"

namespace dfx
{
	class Robin {
	public:

		MemWave wave;

		std::filesystem::path fullPath;
		std::filesystem::path fileName;
		double peak; // In db, as given in the drum font
		double rms;  // In db, as given in the drum font
		unsigned start_frame; // in frames, as given in the drum font
		unsigned end_frame;   // in frames

		Robin(std::string fileName_, double peak_, double rms_, unsigned start_frame_, unsigned end_frame_);
		Robin(const Robin& other);
		Robin(Robin&& other) noexcept;

		Robin& operator=(const Robin& other);
		Robin& operator=(Robin&& other) noexcept;

		void FinishPaths(std::filesystem::path& cumulativePath_);

		bool LoadWave(std::ostream &serr);
	};


	class RobinMgr {
	public:

		std::vector<Robin> robins;
		size_t lastRobinChosen;

	public:

		RobinMgr();
		RobinMgr(const RobinMgr& other);
		RobinMgr(RobinMgr&& other) noexcept;
		virtual ~RobinMgr();

		RobinMgr& operator=(const RobinMgr& other);
		RobinMgr& operator=(RobinMgr&& other) noexcept;

		void FinishPaths(std::filesystem::path& cumulativePath_);

		int LoadWaves(std::ostream &serr);

		Robin& ChooseRobin(); // Lower level

		MemWave& ChooseWave(); // What will be used most of the time.
	};

} // end of namespace