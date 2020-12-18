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
#include <memory>
#include "SampleUtil.h"

#ifdef _DEBUG
#define DFX_DEBUG
#endif

namespace dfx
{
	template<typename T> using MonoFrame = T;

	template<typename T>
	struct StereoFrame
	{
		T left{};
		T right{};

		StereoFrame() = default;
		StereoFrame(T left_, T right_) : left(left_), right(right_) {}
		StereoFrame(const StereoFrame& other) : left(other.left), right(other.right) {}
	};

	// NOTE: If you are going to interpolate, T is best as either float or double.
	// I don't do rounding at the moment.

	template<typename T>
	class FrameBuffer {
	public:

		std::shared_ptr<T[]> samples;
		unsigned nFrames;
		unsigned nChannels;
		unsigned nSamples;
		double dataRate; // In Hz

	public:

		FrameBuffer()
		: samples{}
		, nFrames{}
		, nChannels{}
		, nSamples{}
		, dataRate{ 44100.0 }
		{
		}

		FrameBuffer(unsigned nFrames_, unsigned nChannels_)
		: samples{}
		, dataRate{ 44100.0 }
		{
			Resize(nFrames_, nChannels_);
		}

		FrameBuffer(const FrameBuffer& other)
		: FrameBuffer{}
		{
			Copy(other);
		}

		FrameBuffer(FrameBuffer&& other) noexcept
		: samples(std::move(other.samples))
		, nFrames(other.nFrames)
		, nChannels(other.nChannels)
		, nSamples(other.nSamples)
		, dataRate(other.dataRate)
		{
			// Just keeping move pedantics :)
			other.nFrames = 0;
			other.nChannels = 0;
			other.nSamples = 0;
			other.dataRate = 0;
		}

		virtual ~FrameBuffer()
		{
			samples = nullptr; // Release our claim on the samples
		}

		void operator=(const FrameBuffer& other)
		{
			if (this != &other)
			{
				Copy(other);
			}
		}

		void operator=(FrameBuffer&& other) noexcept
		{
			if (this != &other)
			{
				samples = std::move(other.samples);
				nFrames = other.nFrames;
				nChannels = other.nChannels;
				nSamples = other.nSamples;
				dataRate = other.dataRate;

				// Just keeping move pedantics :)
				other.nFrames = 0;
				other.nChannels = 0;
				other.nSamples = 0;
				other.dataRate = 0;
			}
		}

		void Alias(FrameBuffer& other)
		{
			samples = other.samples; // Aliasing here and grabbing a claim
			nFrames = other.nFrames;
			nChannels = other.nChannels;
			nSamples = other.nSamples;
			dataRate = other.dataRate;
		}

		void Clear()
		{
			for (unsigned i = 0; i < nSamples; i++)
			{
				samples[i] = 0;
			}
		}

		void Copy(const FrameBuffer& other)
		{
			if (this != &other)
			{
				bool clear = false;
				Resize(other.nFrames, other.nChannels, clear);
				memcpy(samples.get(), other.samples.get(), nSamples * sizeof(T));
			}

			dataRate = other.dataRate;
		}

		void Resize(unsigned nFrames_, unsigned nChannels_, bool clear = true)
		{
			if (nFrames_ != nFrames || nChannels_ != nChannels)
			{
				nFrames = nFrames_;
				nChannels = nChannels_;
				nSamples = nFrames * nChannels;
				samples = std::shared_ptr<T[]>(new T[nSamples]); // We auto release claim on old samples
				Clear();
			}
		}

		void SetDataRate(double dataRate_)
		{
			dataRate = dataRate_;
		}

	public:

		MonoFrame<T> GetMonoFrame(unsigned i) const
		{
#ifdef DFX_DEBUG
			if (nChannels != 1)
			{
				throw std::exception("Invalid buffer configuration at FrameBuffer::GetMonoFrame()");
			}

			if (i >= nFrames)
			{
				throw std::exception("Out of bounds at FrameBuffer::MonoSample()");
			}
#endif

			return samples[i];
		}

		StereoFrame<T> GetStereoFrame(unsigned i) const
		{
#ifdef DFX_DEBUG
			if (nChannels != 2)
			{
				throw std::exception("Invalid buffer configuration at FrameBuffer::GetStereoFrame()");
			}

			if (i >= nFrames)
			{
				throw std::exception("Out of bounds at FrameBuffer::StereoSample()");
			}
#endif

			return { samples[i * nChannels], samples[i * nChannels + 1] };
		}

		MonoFrame<T> MonoInterpolate(double pos) const
		{
#ifdef DFX_DEBUG
			if (nChannels != 1)
			{
				throw std::exception("Invalid buffer configuration at FrameBuffer::MonoInterpolate()");
			}

			if (pos < 0.0)
			{
				throw std::exception("Out of bounds at FrameBuffer::MonoInterpolate() - 1");
			}
#endif

			unsigned indx = (unsigned)pos;
			double frac = pos - indx;

			if (frac > 0.0)
			{
				if (indx >= nSamples)
				{
					throw std::exception("Out of bounds at FrameBuffer::interpolate() - 2");
				}
				else if (indx == nSamples - 1)
				{
					// At the last sample, so don't interpolate. Just return the
					// last value.
					return samples[indx];
				}
				else
				{
					// Here to interpolate

					T a = samples[indx];
					T b = samples[indx + 1];

					T output = a;
					output += static_cast<T>(frac * (b - a));

					return output;
				}
			}
			else
			{
#ifdef DFX_DEBUG
				if (indx >= nSamples)
				{
					throw std::exception("Out of bounds at FrameBuffer::MonoInterpolate() - 3");
				}
#endif

				return samples[indx];
			}

		}

		StereoFrame<T> StereoInterpolate(double framePos) const
		{
#ifdef DFX_DEBUG
			if (nChannels != 2)
			{
				throw std::exception("Invalid buffer configuration at FrameBuffer::StereoInterpolate() - 1");
			}

			if (framePos < 0.0)
			{
				throw std::exception("Out of bounds at FrameBuffer::StereoInterpolate() - 2");
			}
#endif
			// This ASSUMES interleaved sampling data. Don't forget that!
			// So the frame, say, at framePos p, is { samples[p], samples[p+1] }

			auto frameIndx = (unsigned)framePos;
			const double frac = framePos - frameIndx;
			const auto firstFrameSample = frameIndx * 2;

			if (frac > 0.0)
			{
				if (frameIndx >= nFrames)
				{
					throw std::exception("Out of bounds at FrameBuffer::StereoInterpolate() - 3");
				}
				else if (frameIndx == nFrames - 1)
				{
					// On last frame, so don't interpolate
					return { samples[firstFrameSample], samples[firstFrameSample + 1] };
				}
				else
				{
					// Okay, then, we must interpolate between frames.
					// Now, two frames are two samples apart.

					auto sampleIndx = firstFrameSample;

					// Let's work on one channel of the frame at a time.
					// First, the left channel

					T a = samples[sampleIndx];
					T b = samples[sampleIndx + 2];

					T output1 = a;
					output1 += static_cast<T>(frac * (b - a));

					// Now, the right channel

					sampleIndx = firstFrameSample + 1;

					a = samples[sampleIndx];
					b = samples[sampleIndx + 2];

					T output2 = a;
					output2 += static_cast<T>(frac * (b - a));

					return { output1, output2 };
				}
			}
			else
			{
				// We are right on the money! No interpolation needed.

#ifdef DFX_DEBUG
				if (frameIndx >= nFrames)
				{
					throw std::exception("Out of bounds at FrameBuffer::StereoInterpolate() - 4");
				}
#endif
				auto sampleIndx = firstFrameSample;
				return { samples[sampleIndx], samples[sampleIndx + 1] };
			}
		}

		T GetAbsMaxOfFrame(unsigned f)
		{
			unsigned sampleIndx = f * nChannels;

			T m = 0;

			for (unsigned i = 0; i < nChannels; i++)
			{
				auto v = std::abs(samples[sampleIndx + i]);
				if (v > m) m = v;
			}

			return m;
		}

		T GetMaxOfFrame(unsigned f)
		{
			unsigned sampleIndx = f * nChannels;

			T m = 0;

			for (unsigned i = 0; i < nChannels; i++)
			{
				auto v = samples[sampleIndx + i];
				if (v > m) m = v;
			}

			return m;
		}

		T GetMinOfFrame(unsigned f)
		{
			unsigned sampleIndx = f * nChannels;
			T m = 0;

			for (unsigned i = 0; i < nChannels; i++)
			{
				auto v = samples[sampleIndx + i];
				if (v < m) m = v;
			}

			return m;
		}

		T GetAvgOfFrame(unsigned f)
		{
			// @@ Not optimized yet

			unsigned sampleIndx = f * nChannels;
			T s = 0;

			for (unsigned i = 0; i < nChannels; i++)
			{
				auto v = samples[sampleIndx + i];
				s += v;
			}

			return (double(s) / nChannels);
		}


		T FindMax(double duration)
		{
			unsigned nFramesToDo;

			if (duration > 0.0)
			{
				// If we want to just look at a portion of the samples

				nFramesToDo = unsigned(duration * dataRate + 0.5);

				if (nFramesToDo > nFrames)
				{
					nFramesToDo = nFrames;
				}
			}
			else
			{
				// If we want to look at all of the samples
				nFramesToDo = nFrames;
			}

			if (nFramesToDo == 0)
			{
				throw std::exception("Range of samples empty");
			}

			size_t nSamplesToDo = nFramesToDo * nChannels;

			// Compute the peak signal seen, either channel

			T neg_peak = 0;
			T pos_peak = 0;

			auto p = samples.get();

			for (size_t i = 0; i < nSamples; i++)
			{
				auto x = *p++;
				if (x < neg_peak) neg_peak = x;
				if (x > pos_peak) pos_peak = x;
			}

			T peak = (-neg_peak) > pos_peak ? -neg_peak : pos_peak;
			return peak;
		}

	};

    //
	// /////////////////////////////////////
	//

	struct WaveStats
	{
		size_t start;
		size_t end;
		double neg_peak;
		double pos_peak;
		double peak;
		double rms;
		int effective_bits;

		WaveStats() = default;

		WaveStats(size_t start_, size_t end_, double neg_peak_, double pos_peak_, double peak_, double rms_, int effective_bits_)
		: start(start_)
		, end(end_)
		, neg_peak(neg_peak_)
		, pos_peak(pos_peak_)
		, peak(peak_)
		, rms(rms_)
		, effective_bits(effective_bits_)
		{

		}
	};

	extern WaveStats ComputeStats(FrameBuffer<double>& buffer, SampleFormat file_type, double file_rate = 0.0, double duration = 0.0);

	// This version isn't worried about the actual peaks found, but instead averages the channels together.
	// It's simpler, and more appropriate for loudness measurements.

	WaveStats ComputeStatsII(FrameBuffer<double>& buffer, SampleFormat data_type, double file_rate, double duration);


} // End of namespace