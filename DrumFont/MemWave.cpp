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

	MemWave::MemWave(const MemWave& other)
	: buff(other.buff)
	, path(other.path)
	, sampleRate(other.sampleRate)
	, deltaTime(other.deltaTime)
	, time(other.time)
	, finished(other.finished)
	, interpolate(other.interpolate)
	{
	}

	MemWave::MemWave(MemWave&& other) noexcept
	: buff(std::move(other.buff))
	, path(std::move(other.path))
	, sampleRate(other.sampleRate)
	, deltaTime(other.deltaTime)
	, time(other.time)
	, finished(other.finished)
	, interpolate(other.interpolate)
	{
		// Just keeping move pedantics :)

		other.sampleRate = 0;
		other.deltaTime = 0;
		other.time = 0;
		other.finished = false;
		other.interpolate = false;
	}

	void MemWave::operator=(const MemWave& other)
	{
		if (this != &other)
		{
			buff = other.buff;
			path = other.path;
			sampleRate = other.sampleRate;
			deltaTime = other.deltaTime;
			time = other.time;
			finished = other.finished;
			interpolate = other.interpolate;
		}
	}

	void MemWave::operator=(MemWave&& other) noexcept
	{
		buff = std::move(other.buff);
		path = std::move(other.path);
		sampleRate = other.sampleRate;
		deltaTime = other.deltaTime;
		time = other.time;
		finished = other.finished;
		interpolate = other.interpolate;

		// Just keeping move pedantics :)

		other.sampleRate = 0;
		other.deltaTime = 0;
		other.time = 0;
		other.finished = false;
		other.interpolate = false;
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
		deltaTime = buff.dataRate / sampleRate;
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

	MonoFrame<double> MemWave::MonoTick()
	{
#ifdef DFX_DEBUG
		if (buff.nFrames != 1)
		{
			throw std::exception("Buffer isn't in mono. MemWave::MonoTick()");
		}
#endif

		if (finished)
		{
			return 0.0;
		}

		unsigned nFrames = buff.nFrames;

		if (time > nFrames - 1.0)
		{
			time = nFrames - 1.0;
			finished = true;
			return 0.0;
		}

		MonoFrame<double> sample;

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

		return sample;
	}


	StereoFrame<double> MemWave::StereoTick()
	{
#ifdef DFX_DEBUG
		if (buff.nFrames != 2)
		{
			throw std::exception("Buffer isn't in stereo. MemWave::StereoTick()");
		}
#endif

		if (finished)
		{
			return { 0.0, 0.0 };
		}

		unsigned nFrames = buff.nFrames;

		if (time > nFrames - 1.0)
		{
			time = nFrames - 1.0;
			finished = true;
			return { 0.0, 0.0 };
		}

		StereoFrame<double> frame;

		if (interpolate)
		{
			frame = buff.StereoInterpolate(time);
		}
		else
		{
			frame = buff.GetStereoFrame(static_cast<unsigned>(time));
		}

		// Get ready for next go round

		time += deltaTime;

		return frame;
	}

	bool MemWave::IsFinished()
	{
		return true;
	}

} // end of namspace