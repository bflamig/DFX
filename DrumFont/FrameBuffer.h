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

namespace dfx
{
	template<typename T> using MonoFrame = T;
	template<typename T> using StereoFrame = std::pair<T, T>;

	template<typename T>
	class FrameBuffer {
	public:

		std::shared_ptr<T[]> samples;
		unsigned nFrames;
		unsigned nSamples;
		unsigned nChannels;
		double dataRate; // In Hz

	public:

		FrameBuffer()
		: samples{}
		, nFrames{}
		, nSamples{}
		, nChannels{}
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
		: samples(other.samples)
		, nFrames(other.nFrames)
		, nSamples(other.nSamples)
		, nChannels(other.nChannels)
		, dataRate(other.dataRate)
		{
		}

		FrameBuffer(FrameBuffer&& other) noexcept
		: samples(std::move(other.samples))
		, nFrames(other.nFrames)
		, nSamples(other.nSamples)
		, nChannels(other.nChannels)
		, dataRate(other.dataRate)
		{
			// Just keeping move pedantics :)
			other.nFrames = 0;
			other.nSamples = 0;
			other.nChannels = 0;
			other.dataRate =  0;
		}

		virtual ~FrameBuffer() 
		{
			samples = nullptr; // Release our claim on the samples
		}

		void operator=(const FrameBuffer& other)
		{
			if (this != &other)
			{
				samples = other.samples;
				nFrames = other.nFrames;
				nSamples = other.nSamples;
				nChannels = other.nChannels;
				dataRate = other.dataRate;
			}
		}

		void operator=(FrameBuffer&& other) noexcept
		{
			if (this != &other)
			{
				samples = std::move(other.samples);
				nFrames = other.nFrames;
				nSamples = other.nSamples;
				nChannels = other.nChannels;
				dataRate = other.dataRate;

				// Just keeping move pedantics :)
				other.nFrames = 0;
				other.nSamples = 0;
				other.nChannels = 0;
				other.dataRate = 0;
			}
		}


		void Clear()
		{
			for (unsigned i = 0; i < nSamples; i++)
			{
				samples[i] = 0.0;
			}
		}

		void Resize(unsigned nFrames_, unsigned nChannels_)
		{
			if (nFrames_ != nFrames || nChannels_ != nChannels)
			{
				nFrames = nFrames_;
				nChannels = nChannels_;
				nSamples = nFrames * nChannels;
				samples = new T[nSamples]; // We auto release claim on old samples
				Clear();
			}
		}

		void Alias(FrameBuffer& other)
		{
			samples = other.samples; // Aliasing here and grabbing a claim
			nFrames = other.nFrames;
			nChannels = other.nChannels;
			nSamples = other.nSamples;
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
					output += frac * (b - a);

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

		StereoFrame<T> StereoInterpolate(double pos) const
		{
#ifdef DFX_DEBUG
			if (nChannels != 2)
			{
				throw std::exception("Invalid buffer configuration at FrameBuffer::StereoInterpolate() - 1");
			}

			if (pos < 0.0)
			{
				throw std::exception("Out of bounds at FrameBuffer::StereoInterpolate() - 2");
			}
#endif
			unsigned indx = (unsigned)pos;
			double frac = pos - indx;

			if (frac > 0.0)
			{
				if (indx >= nFrames)
				{
					throw std::exception("Out of bounds at FrameBuffer::StereoInterpolate() - 3");
				}
				else if (indx == nFrames - 1)
				{
					// On last frame, so don't interpolate
					return { samples[indx], samples[indx + 1] };
				}
				else
				{
					// Here to interpolate

					T a = samples[indx];
					T b = samples[++indx];

					T output1 = a;
					output1 += frac * (b - a);

					a = samples[++indx];
					b = samples[++indx];

					T output2 = a;
					output2 += frac * (b - a);

					return { output1, output2 };
				}
			}
			else
			{
#ifdef DFX_DEBUG
				if (indx >= nFrames)
				{
					throw std::exception("Out of bounds at FrameBuffer::StereoInterpolate() - 4");
				}
#endif
				return { samples[indx], samples[indx + 1] };
			}

		}
	};

} // End of namespace