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

namespace dfx
{

#if 0
	template<typename T>
	struct MonoFrame
	{
		T sample;

		MonoFrame() = default;

		MonoFrame(const MonoFrame& other) : sample(other) { }

		explicit MonoFrame(const T& sample_) : sample(sample_) { }

		int NumChannels() { return 1; }

		static MonoFrame Lerp(const MonoFrame &a, const MonoFrame &b, double alpha)
		{
			T output = a;
			output += alpha * (b - a);
			return MonoFrame(output);
		}
	};

	template<typename T>
	struct StereoFrame
	{
		T left_sample;
		T right_sample;

		StereoFrame() = default;

		StereoFrame(const StereoFrame& other) 
		: left_sample(other.left_sample), right_sample(other.right_sample) 
		{ 
		}

		StereoFrame(const T& left, const T& right) 
		: left_sample(left), right_sample(right) 
		{ 
		}

		int NumChannels() { return 2; }

		static StereoFrame Lerp(const StereoFrame& a, const StereoFrame& b, double alpha)
		{
			T output_l = a.left_sample;
			output_l += alpha * (b.left_sample - a.left_sample);

			T output_r = a.right_sample;
			output_r += alpha * (b.right_sample - a.right_sample);

			return StereoFrame(output_l, output_r);
		}
	};

#endif

	template<typename T>
	class FrameBuffer {
	public:

		std::shared_ptr<T[]> samples;
		unsigned nFrames;
		unsigned nSamples;
		unsigned nChannels;

		double sampleRate; // In Hz

	public:

		FrameBuffer()
		: samples{}
		, nFrames{}
		, nSamples{}
		, nChannels{}
		, sampleRate{ 44100.0 }
		{
		}

		FrameBuffer(unsigned nFrames_, unsigned nChannels_)
		: samples{}
		, sampleRate{ 44100.0 }
		{
			Resize(nFrames_, nChannels_);
		}

		virtual ~FrameBuffer() 
		{
			samples = nullptr; // Release our claim on the samples
		}

		void Resize(unsigned nFrames_, unsigned nChannels_)
		{
			if (nFrames_ != nFrames || nChannels_ != nChannels)
			{
				nFrames = nFrames_;
				nChannels = nChannels_;
				nSamples = nFrames * nChannels;
				samples = new T[nSamples]; // We auto release claim on old samples
			}
		}

		void Alias(FrameBuffer& other)
		{
			samples = other.samples; // Aliasing here and grabbing a claim
			nFrames = other.nFrames;
			nChannels = other.nChannels;
			nSamples = other.nSamples;
		}

		void SetSamplingRate(double sampleRate_)
		{
			sampleRate = sampleRate_;
		}

	public:

		T GetMonoFrame(unsigned i) const
		{
			if (i >= nSamples)
			{
				throw std::exception("Out of bounds at FrameBuffer::MonoSample() const");
			}

			return samples[i];
		}

		std::pair<T, T> GetStereoFrame(unsigned i) const
		{
			if (nChannels != 2)
			{
				throw std::exception("Invalid buffer configuration at FrameBuffer::StereoSample() const");
			}

			if (i >= nFrames)
			{
				throw std::exception("Out of bounds at FrameBuffer::StereoSample() const");
			}

			return { samples[i * nChannels], samples[i * nChannels + 1] };
		}

		T MonoInterpolate(double pos) const
		{
			if (pos < 0.0)
			{
				throw std::exception("Out of bounds at FrameBuffer::MonoInterpolate() - 1");
			}

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
				if (indx >= nSamples)
				{
					throw std::exception("Out of bounds at FrameBuffer::MonoInterpolate() - 3");
				}

				return samples[indx];
			}

		}

		std::pair<T, T> StereoInterpolate(double pos) const
		{
			if (nChannels != 2)
			{
				throw std::exception("Invalid buffer configuration at FrameBuffer::StereoInterpolate() - 1");
			}

			if (pos < 0.0)
			{
				throw std::exception("Out of bounds at FrameBuffer::StereoInterpolate() - 2");
			}

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
				if (indx >= nFrames)
				{
					throw std::exception("Out of bounds at FrameBuffer::StereoInterpolate() - 4");
				}

				return { samples[indx], samples[indx + 1] };
			}

		}
	};




} // End of namespace