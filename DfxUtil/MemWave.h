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
#include "SoundFile.h"

namespace dfx
{
	class MemWave {
	public:

		SoundFile sound_file;
		FrameBuffer<double> buff;
		std::filesystem::path path;

		double sampleRate;   // In Hz.
		double time;         // Floating posn through the frames
		double deltaTime;    // buff.dataRate / sampleRate
		bool finished;       // Time's up! (At end of the waves.)

		bool interpolate;

	public:

		MemWave();
		virtual ~MemWave() {}

		MemWave(const MemWave& other);
		MemWave(MemWave&& other) noexcept;

		void operator=(const MemWave& other);
		void operator=(MemWave&& other) noexcept;

		void Clear();

		bool Load(const std::filesystem::path& path_, unsigned start_frame = 0, unsigned end_frame = 0, double scale_factor_code = 1);
		bool LoadRaw(const std::filesystem::path& path_, unsigned nChannels_, SampleFormat format_, double fileRate_);

		void Reset();
		void AliasSamples(MemWave& other);

		void SetRate(double sampleRate_);
		void AddTime(double delta_);

		MonoFrame<double> MonoTick();
		StereoFrame<double> StereoTick();
		bool IsFinished();
	};

} // end of namespace
