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

#include "MemWave.h"

namespace dfx
{
	MemWave::MemWave()
	: buff{}
	, path{}
	, sampleRate(44100.0)
	, deltaTime{ 1.0 }
	, time{}
	, finished{}
	, interpolate{}
	{

	}

	void MemWave::Reset()
	{
		time = 0.0;
		finished = false;
	}

	void MemWave::Load(const std::filesystem::path& path_)
	{
		path = path_;
		// fill buffer, set numFrames; data rate, etc 
	}

	void MemWave::AliasSamples(MemWave& other)
	{
		buff.Alias(other.buff);
	}

	void MemWave::SetRate(double sampleRate_)
	{
		sampleRate = sampleRate_;
		deltaTime = buff.sampleRate / sampleRate;
		interpolate = fmod(deltaTime, 1.0) != 0.0 ? true : false;
	}


	void MemWave::AddTime(double delta_)
	{
		time += delta_;

		if (time < 0.0) time = 0.0;

		unsigned nFrames = buff.nFrames;

		if (time > nFrames - 1.0)
		{
			time = nFrames - 1.0;
			finished = true;
		}
	}

	void MemWave::MonoTick(double& sample)
	{
		if (buff.nFrames != 1)
		{
			throw std::exception("Buffer isn't in mono. MemWave::MonoTick()");
		}

		if (finished)
		{
			sample = 0.0; 
			return;
		}

		unsigned nFrames = buff.nFrames;

		if (time > nFrames - 1.0)
		{
			time = nFrames - 1.0;
			finished = true;
			sample = 0.0;
			return;
		}

		if (interpolate)
		{
			sample = buff.MonoInterpolate(time);
		}
		else
		{
			sample = buff.GetMonoFrame(static_cast<unsigned>(time));
		}

		// Get ready for next go round

		time += deltaTime;
	}


	void MemWave::StereoTick(double& left, double& right)
	{
		if (buff.nFrames != 2)
		{
			throw std::exception("Buffer isn't in stereo. MemWave::StereoTick()");
		}

		if (finished)
		{
			left = 0.0; right = 0.0;
			return;
		}

		unsigned nFrames = buff.nFrames;

		if (time > nFrames - 1.0)
		{
			time = nFrames - 1.0;
			finished = true;
			left = 0.0; right = 0.0;
			return;
		}

		if (interpolate)
		{
			auto fred = buff.StereoInterpolate(time);
			left = fred.first;
			right = fred.second;
		}
		else
		{
			auto fred = buff.GetStereoFrame(static_cast<unsigned>(time));
			left = fred.first;
			right = fred.second;
		}

		// Get ready for next go round

		time += deltaTime;
	}

	bool MemWave::IsFinished()
	{
		return true;
	}

} // end of namspace