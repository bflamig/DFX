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
#include "MultiLayeredDrum.h"

namespace dfx
{

	MultiLayeredDrum::MultiLayeredDrum(const std::string& name_, const std::string& drumPath_, int midiNote_)
		: cumulativePath()
		, drumPath(drumPath_)
		, name(name_)
		, velocityLayers()
		, midiNote(midiNote_)
	{

	}

	MultiLayeredDrum::MultiLayeredDrum(const MultiLayeredDrum& other)
		: cumulativePath(other.cumulativePath)
		, drumPath(other.drumPath)
		, name(other.name)
		, velocityLayers(other.velocityLayers)
		, midiNote(other.midiNote)
	{
		// Copy constructor
	}


	MultiLayeredDrum::MultiLayeredDrum(MultiLayeredDrum&& other) noexcept
		: cumulativePath(std::move(other.cumulativePath))
		, drumPath(std::move(other.drumPath))
		, name(std::move(other.name))
		, velocityLayers(std::move(other.velocityLayers))
		, midiNote(other.midiNote)
	{
		// Move constructor
	}

	void MultiLayeredDrum::SortLayers()
	{
		// First, take all the velocity codes (aka min velocity values) from
		// the instrument, and begin to populate the velocity table.

		std::sort(velocityLayers.begin(), velocityLayers.end(),
			[](const std::shared_ptr<VelocityLayer>& a, const std::shared_ptr<VelocityLayer>& b) // Hmm, where's the return type?
			{
				return a->vrange.iMinVel < b->vrange.iMinVel;
			}
		);

		// Check for duplicates. If see any, we're outta here

		const auto nlayers = velocityLayers.size();

		int prev_vel_code = velocityLayers[0]->vrange.iMinVel;

		for (size_t i = 1; i < nlayers; i++)
		{
			int next_vel_code = velocityLayers[i]->vrange.iMinVel;
			if (next_vel_code == prev_vel_code)
			{
				//oStream_ << "MultiLayeredDrum::SortVelocityRanges(): There is a least one duplicate velocity code in the set.\n";
				//handleError(StkError::FILE_CONFIGURATION);
			}
			prev_vel_code = next_vel_code;
		}

		// Set the max vel's.

		// But first, let's inspect the first layer. If its minimum velocity
		// isn't zero, then make it. It kinda needs to be.

		if (velocityLayers[0]->vrange.iMinVel != 0)
		{
			velocityLayers[0]->vrange.iMinVel = 0;
		}

		// onward

		for (size_t i = 0; i < nlayers - 1; i++)
		{
			velocityLayers[i]->vrange.iMaxVel = velocityLayers[i + 1]->vrange.iMinVel - 1;
		}

		// Set the maximum velocity of the last element

		velocityLayers[nlayers - 1]->vrange.iMaxVel = 127; // by definition

		// Okay, set the floating point scaled versions of these things

		for (auto& vl : velocityLayers)
		{
			vl->vrange.fMinVel = (double)vl->vrange.iMinVel / 127.0;
			vl->vrange.fMaxVel = (double)vl->vrange.iMaxVel / 127.0;
		}

		// Whew! We're done!
	}

	int MultiLayeredDrum::FindVelocityLayer(int vel)         // Mostly for debugging
	{
		int idx = -1;

		if (vel < 0 || vel > 127)
		{
			//oStream_ << "MultiLayeredDrum::SelectVelocityLayer(): Velocity " << vel << " out of range.\n";
			//handleError(StkError::FUNCTION_ARGUMENT);
		}

		auto n = velocityLayers.size();

		for (size_t i = 0; i < n; i++)
		{
			const auto& e = velocityLayers[i]->vrange;
			if ((vel >= e.iMinVel) && (vel <= e.iMaxVel)) return i; //  velocityLayers[i].vrange.robinsIdx;
		}

		//oStream_ << "MultiLayeredDrum::SelectVelocityLayer(): Couldn't find a range for velocity " << vel << "\n";
		//handleError(StkError::UNSPECIFIED);
		//return -1; // Won't get here, but whatever

		return idx;
	}

	int MultiLayeredDrum::FindVelocityLayer(double vel)
	{
		int idx = -1;

		if (vel < 0.0 || vel > 1.0)
		{
			//oStream_ << "MultiLayeredDrum::SelectVelocityLayer(): Velocity " << vel << " out of range.\n";
			//handleError(StkError::FUNCTION_ARGUMENT);
		}

		auto n = velocityLayers.size();

		for (size_t i = 0; i < n; i++)
		{
			const auto& e = velocityLayers[i]->vrange;
			if ((vel >= e.fMinVel) && (vel <= e.fMaxVel)) return i; //  velocityLayers[i].vrange.robinsIdx;
		}

		//oStream_ << "MultiLayeredDrum::SelectVelocityLayer(): Couldn't find a range for velocity " << vel << "\n";
		//handleError(StkError::UNSPECIFIED);
		//return -1; // Won't get here, but whatever

		return idx;
	}


	RobinMgr& MultiLayeredDrum::SelectVelocityLayer(int vel)  // Mostly for debugging
	{
		auto r = FindVelocityLayer(vel);
		return velocityLayers[r]->robinMgr;
	}

	RobinMgr& MultiLayeredDrum::SelectVelocityLayer(double vel)
	{
		auto r = FindVelocityLayer(vel);
		return velocityLayers[r]->robinMgr;
	}

	MemWave& MultiLayeredDrum::ChooseWave(int vel) // Mostly for debugging
	{
		auto& rm = SelectVelocityLayer(vel);
		auto& mw = rm.ChooseWave();
		return mw;
	}



	MemWave& MultiLayeredDrum::ChooseWave(double vel)
	{
		auto& rm = SelectVelocityLayer(vel);
		auto& mw = rm.ChooseWave();
		return mw;
	}


	void MultiLayeredDrum::LoadWaves()
	{
		for (auto& lp : velocityLayers)
		{
			lp->LoadWaves();
		}
	}

}