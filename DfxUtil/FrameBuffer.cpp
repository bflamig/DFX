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

#include "FrameBuffer.h"

namespace dfx
{
	template<typename T>
	int EffectiveIntegerBits(T x)
	{
		int nd = 0;

		while (true)
		{
			if (x == 0) return nd;
			x /= 2;
			nd++;
		}

		return nd;
	}

	int EffectiveBits(double x, SampleFormat data_type)
	{
		switch (data_type)
		{
			case SampleFormat::SINT16:
			{
				auto d = int16_t(x * 32767.5 - 0.5);
				return EffectiveIntegerBits(d);

			}
			break;
			case SampleFormat::SINT24:
			{
				auto d = int32_t(x * 8388607.5 - 0.5);
				return EffectiveIntegerBits(d);

			}
			break;
			case SampleFormat::SINT32:
			{
				auto d = int32_t(x * 2147483647.5 - 0.5);
				return EffectiveIntegerBits(d);
			}
			break;
			case SampleFormat::FLOAT32:
			case SampleFormat::FLOAT64:
			{
				x += 0.5;
				x /= 2147483647.5;
				x = int32_t(x);
				return EffectiveIntegerBits(x);
			}
		}

		throw std::exception("Invalid sample format");
	}

	// This is an attempt to judge the practical "extent" and "volume" of an audio signal.
	// We only do this for frame buffers of doubles.

	WaveStats ComputeStats(FrameBuffer<double>& buffer, SampleFormat data_type, double file_rate, double duration)
	{
		unsigned nframes;

		if (duration > 0.0)
		{
			// If we want to just look at a portion of the samples

			nframes = unsigned(duration * file_rate + 0.5);

			if (nframes > buffer.nFrames)
			{
				nframes = buffer.nFrames;
			}
		}
		else
		{
			// If we want to look at all of the samples

			nframes = buffer.nFrames;
		}

		if (nframes == 0)
		{
			throw std::exception("Buffer empty");
		}

		// Scan from the start till we see a signal above a threshold;

		constexpr double start_threshold = 0.0001;

		unsigned startFrame;

		for (startFrame = 0; startFrame < nframes; ++startFrame)
		{
			auto x = std::abs(buffer.GetAbsMaxOfFrame(startFrame));
			if (x >= start_threshold) break;
		}

		if (startFrame == nframes)
		{
			// Nothing above threshold, so we'll have to punt
			startFrame = 0;
		}

		// Scan from the end till we see a signal above a threshold

		constexpr double end_threshold = 0.0001;

		unsigned endFrame = nframes;

		while (1)
		{
			auto x = std::abs(buffer.GetAbsMaxOfFrame(--endFrame));
			if (x >= end_threshold) break;
			if (endFrame == 0)
			{
				// Nothing above threshold, so we'll have to punt
				endFrame = nframes;
				break;
			}
		}

		const auto nframes_chunk = nframes / 100;

		// Compute the peak signal seen, either channel

		double neg_peak = 0.0;
		double pos_peak = 0.0;

		for (unsigned i = startFrame; i < endFrame; i++)
		{
			auto x = buffer.GetMinOfFrame(i);
			if (x < neg_peak) neg_peak = x;


			auto y = buffer.GetMaxOfFrame(i);
			if (y > pos_peak) pos_peak = y;
		}

		double peak = std::abs(neg_peak) > pos_peak ? std::abs(neg_peak) : pos_peak;

		const double threshold = peak / 100.0;

		// Accumulate sum squares statistics

		int accum_k = 0;

		double sum_squared = 0.0;
		double fallback_sum_squared = 0.0;

		unsigned i = startFrame;
		while (i < endFrame)
		{
			double chunk_sum_squared = 0.0;

			unsigned k;

			for (k = 0; k < nframes_chunk; k++)
			{
				// We're just incorporating all channels in the rms
				// we'll fix that afterwards

				auto si = k * buffer.nChannels;

				for (size_t j = 0; j < buffer.nChannels; j++)
				{
					auto d = std::abs(buffer.samples[si + j]);
					chunk_sum_squared += d * d;
				}

				if (i++ >= endFrame) break;
			}

			auto s = chunk_sum_squared / buffer.nChannels;
			s /= k;
			s = sqrt(s);

			if (s >= threshold)
			{
				sum_squared += chunk_sum_squared;
				accum_k += k;
			}

			fallback_sum_squared += chunk_sum_squared;
		}

		double rms;

		if (accum_k > 0)
		{
			rms = sum_squared / buffer.nChannels;
			rms /= accum_k;
		}
		else
		{
			rms = fallback_sum_squared / buffer.nChannels;
			rms /= nframes;
		}

		rms = sqrt(rms);

		int effective_bits = EffectiveBits(peak, data_type);
		return WaveStats(startFrame, endFrame, neg_peak, pos_peak, peak, rms, effective_bits);
	}


	// This version isn't worried about the actual peaks found, but instead averages the channels together.
	// It's simpler, and more appropriate for loudness measurements.

	WaveStats ComputeStatsII(FrameBuffer<double>& buffer, SampleFormat data_type, double file_rate, double duration)
	{
		unsigned nframes;

		if (duration > 0.0)
		{
			// If we want to just look at a portion of the samples

			nframes = unsigned(duration * file_rate + 0.5);

			if (nframes > buffer.nFrames)
			{
				nframes = buffer.nFrames;
			}
		}
		else
		{
			// If we want to look at all of the samples

			nframes = buffer.nFrames;
		}

		if (nframes == 0)
		{
			throw std::exception("Buffer empty");
		}

		// Scan from the start till we see a signal above a threshold;

		constexpr double start_threshold = 0.0001;

		unsigned startFrame;

		for (startFrame = 0; startFrame < nframes; ++startFrame)
		{
			auto x = buffer.GetAvgOfFrame(startFrame);
			if (x >= start_threshold) break;
		}

		if (startFrame == nframes)
		{
			// Nothing above threshold, so we'll have to punt
			startFrame = 0;
		}

		// Scan from the end till we see a signal above a threshold

		constexpr double end_threshold = 0.0001;

		unsigned endFrame = nframes;

		while (1)
		{
			auto x = buffer.GetAvgOfFrame(--endFrame);
			if (x >= end_threshold) break;
			if (endFrame == 0)
			{
				// Nothing above threshold, so we'll have to punt
				endFrame = nframes;
				break;
			}
		}

		const auto nframes_chunk = nframes / 100;

		// Compute the max signal seen, either channel

		double neg_max = 0.0;
		double pos_max = 0.0;

		for (unsigned i = startFrame; i < endFrame; i++)
		{
			auto x = buffer.GetAvgOfFrame(i);
			if (x < neg_max) neg_max = x;
			if (x > pos_max) pos_max = x;
		}

		double peak = std::abs(neg_max) > pos_max ? std::abs(neg_max) : pos_max;

		const double threshold = peak / 100.0;

#if 0
		// Accumulate sum squares statistics

		int accum_k = 0;

		double sum_squared = 0.0;
		double fallback_sum_squared = 0.0;

		unsigned i = startFrame;
		while (i < endFrame)
		{
			double chunk_sum_squared = 0.0;

			unsigned k;

			for (k = 0; k < nframes_chunk; k++)
			{
				// We're just incorporating all channels in the rms
				// we'll fix that afterwards

				auto d = buffer.GetAvgOfFrame(i);
				chunk_sum_squared += d * d;

				if (i++ >= endFrame) break;
			}

			auto s = chunk_sum_squared / buffer.nChannels;
			s /= k;
			s = sqrt(s);

			if (s >= threshold)
			{
				sum_squared += chunk_sum_squared;
				accum_k += k;
			}

			fallback_sum_squared += chunk_sum_squared;
		}

		double rms;

		if (accum_k > 0)
		{
			rms = sum_squared / buffer.nChannels;
			rms /= accum_k;
		}
		else
		{
			rms = fallback_sum_squared / buffer.nChannels;
			rms /= nframes;
		}

		rms = sqrt(rms);

#else
		double rms;
		rms = ComputePeakRms(buffer, 2.0); // @@ TODO HACK FIXE ASAP// duration);
#endif

		int effective_bits = EffectiveBits(peak, data_type);
		return WaveStats(startFrame, endFrame, neg_max, pos_max, peak, rms, effective_bits);
	}


	//
	// ////////////////////////////////////////////////////////
	// From WaveSplitter work

	double ComputePeakRms(FrameBuffer<double>& fb, double duration)
	{
		unsigned startFrame = 0;
		unsigned endFrame = startFrame + unsigned(fb.dataRate * duration);
		if (endFrame >= fb.nFrames) endFrame = fb.nFrames - 1;
		return ComputePeakRms(fb, startFrame, endFrame);
	}

	double ComputePeakRms(FrameBuffer<double> &fb, unsigned startFrame, unsigned endFrame)
	{
		// endFrame should be one past last frame of interest
		// Right now, 24 bit only

		unsigned extent = endFrame - startFrame; // @@ TODO: bounds check!

		unsigned wd = unsigned(fb.dataRate * 20e-3); // roughly a 20 ms window

		double peakRms = 0.0;

		for (unsigned j = 0; j < extent; j++)
		{
			double S = 0.0;

			for (unsigned i = 0; i < wd; i++)
			{
				unsigned k = i + wd * j;

				if (k < extent)
				{
					double W = fb.GetAbsMaxOfFrame(k + startFrame);
					W = W * W / wd;
					S += W;
				}
				else break;
			}

			double Rms = sqrt(S);
			if (Rms > peakRms) peakRms = Rms;
		}

		return peakRms;
	}

} // End of namespace