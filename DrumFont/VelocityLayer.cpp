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

#include "VelocityLayer.h"

namespace dfx
{

	VelocityRange::VelocityRange()
	{
		clear();
	}

	VelocityRange::VelocityRange(int velCode_)
	: VelocityRange()
	{
		velCode = velCode_;
		iMinVel = velCode;
		iMaxVel = velCode;
		fMinVel = velCode / 127.0;
		fMaxVel = velCode / 127.0;
	}

	void VelocityRange::clear()
	{
		velCode = 0;
		iMinVel = 0;
		iMaxVel = 0;
		fMinVel = 0;
		fMaxVel = 0;
	}

	///

	VelocityLayer::VelocityLayer()
	: cumulativePath{}
	, localPath{}
	, vrange{}
	, robinMgr{}
	{

	}


	VelocityLayer::VelocityLayer(std::string& localPath_, int vel_code_)
	: VelocityLayer{}
	{
		localPath = localPath_;
		localPath = localPath.generic_string();
		vrange.velCode = vel_code_;
		vrange.iMinVel = vel_code_;
	}

	VelocityLayer::VelocityLayer(const VelocityLayer& other)
	: cumulativePath(other.cumulativePath)
	, localPath(other.localPath)
	, vrange(other.vrange)
	, robinMgr(other.robinMgr)
	{
	}

	VelocityLayer::VelocityLayer(VelocityLayer&& other) noexcept
	: cumulativePath(std::move(other.cumulativePath))
	, localPath(std::move(other.localPath))
	, vrange(other.vrange)
	, robinMgr(std::move(other.robinMgr))
	{
		other.vrange.clear(); // just keeping move pedantics :)
	}


	void VelocityLayer::operator=(const VelocityLayer& other)
	{
		if (this != &other)
		{
			cumulativePath = other.cumulativePath;
			localPath = other.localPath;
			vrange = other.vrange;
			robinMgr = other.robinMgr;
		}
	}

	void VelocityLayer::operator=(VelocityLayer&& other) noexcept
	{
		if (this != &other)
		{
			cumulativePath = std::move(other.cumulativePath);
			localPath = std::move(other.localPath);
			vrange = other.vrange;
			robinMgr = std::move(other.robinMgr);
			other.vrange.clear(); // just keeping move pedantics :)
		}
	}



	void VelocityLayer::FinishPaths(std::filesystem::path& cumulativePath_)
	{
		cumulativePath = cumulativePath_;
		cumulativePath /= localPath;
		cumulativePath = cumulativePath.generic_string();
		robinMgr.FinishPaths(cumulativePath);
	}

	void VelocityLayer::LoadWaves()
	{
		robinMgr.LoadWaves();
	}

} // end of namespace
