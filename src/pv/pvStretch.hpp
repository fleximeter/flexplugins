/*
File: pvStretch.hpp
Author: Jeff Martin

Description:
A phase vocoder time stretcher.

Copyright © 2026 by Jeffrey Martin. All rights reserved.
Website: https://www.jeffreymartincomposer.com

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#pragma once
#include "SC_Unit.h"
#include "FFT_UGens.h"
#include "peakFinder.hpp"

/// Stores the state of a PV_PlayBufStretch UGen instance
struct PV_PlayBufStretch : public Unit {
    /// The index of the buffer with STFT data
    float m_fbufnum;

    /// The buffer with STFT data
    SndBuf *m_buf;

    /// The most recent output STFT frame
    SCPolarBuf *m_outFramePrev;

    /// The next STFT frame after the current position
    SCPolarBuf *m_frameNext;
    
    /// The STFT frame right before the current position
    SCPolarBuf *m_framePrev1;
    
    /// The STFT frame two positions before the current position
    SCPolarBuf *m_framePrev2;

    /// The peak finding utility for the Laroche/Dolson stretching algorithm
    PeakFinder *m_peakFinder;

    /// Between 0 and 1; represents the start position of playback.
    /// If it jumps during playback, playback will be restarted
    /// at the new m_startPos.
    float m_startPos;
    
    /// A fractional STFT frame index. Unlike m_startPos, it corresponds to the 
    /// integer index of the current STFT frame (from 0 to M-1).
    /// Used for interpolating position.
    float m_pos;

    /// For the first frame, we need to read phase data directly
    /// instead of computing it.
    bool m_firstFrame;
};

/// Initializes a new PV_PlayBufStretch UGen
void PV_PlayBufStretch_Ctor(PV_PlayBufStretch *unit);

/// Frees memory created by the PV_PlayBufStretch UGen
void PV_PlayBufStretch_Dtor(PV_PlayBufStretch *unit);

/// Computes the next FFT frame requested by the PV_PlayBufStretch UGen
void PV_PlayBufStretch_next(PV_PlayBufStretch *unit, int inNumSamples);

/// Computes a single frame of STFT data for time stretching.
/// The assumption is that we are positioned exactly at `frame`, and we therefore
/// just need framePrev to compute the instantaneous frequency. We also do not
/// need to perform any magnitude or frequency interpolation.
///
/// \param frame The current STFT frame
/// \param framePrev The previous STFT frame
/// \param [out] outFrame The output STFT frame
/// \param outFramePrev The previously computed output STFT frame
/// \param fftSize The FFT size
/// \param hopSize The hop size
void Stretch2(
    const SCPolarBuf *frame, 
    const SCPolarBuf *framePrev, 
    SCPolarBuf *outFrame, 
    const SCPolarBuf *outFramePrev, 
    size_t fftSize, 
    size_t hopSize);

/// Computes a single frame of STFT data for time stretching.
/// The assumption is that we are positioned exactly at `frame`, and we therefore
/// just need framePrev to compute the instantaneous frequency. We also do not
/// need to perform any magnitude or frequency interpolation.
///
/// This version uses Miller Puckette's phase locking.
///
/// \param frame The current STFT frame
/// \param framePrev The previous STFT frame
/// \param [out] outFrame The output STFT frame
/// \param outFramePrev The previously computed output STFT frame
/// \param fftSize The FFT size
/// \param hopSize The hop size
void Stretch2Puckette(
    const SCPolarBuf *frame, 
    const SCPolarBuf *framePrev, 
    SCPolarBuf *outFrame, 
    const SCPolarBuf *outFramePrev, 
    size_t fftSize, 
    size_t hopSize);

/// Computes a single frame of STFT data for time stretching.
/// The assumption is that we are positioned between framePrev1 and frameNext.
/// This means we will need to interpolate frequency data. So we will need to
/// compute two frequencies for each bin, and that means we need three STFT frames.
///
/// \param frameNext The next STFT frame
/// \param framePrev1 The previous STFT frame
/// \param framePrev2 The previous STFT frame before that (required for instantaneous frequency interpolation)
/// \param [out] outFrame The output STFT frame
/// \param outFramePrev The previously computed output STFT frame
/// \param pos The position between framePrev1 and frameNext (0 < pos < 1)
/// \param fftSize The FFT size
/// \param hopSize The hop size
void Stretch3(
    const SCPolarBuf *frameNext, 
    const SCPolarBuf *framePrev1,
    const SCPolarBuf *framePrev2, 
    SCPolarBuf *outFrame,
    const SCPolarBuf *outFramePrev,
    float pos,
    size_t fftSize, 
    size_t hopSize);

/// Computes a single frame of STFT data for time stretching.
/// The assumption is that we are positioned between framePrev1 and frameNext.
/// This means we will need to interpolate frequency data. So we will need to
/// compute two frequencies for each bin, and that means we need three STFT frames.
///
/// This version uses Miller Puckette's phase locking.
///
/// \param frameNext The next STFT frame
/// \param framePrev1 The previous STFT frame
/// \param framePrev2 The previous STFT frame before that (required for instantaneous frequency interpolation)
/// \param [out] outFrame The output STFT frame
/// \param outFramePrev The previously computed output STFT frame
/// \param pos The position between framePrev1 and frameNext (0 < pos < 1)
/// \param fftSize The FFT size
/// \param hopSize The hop size
void Stretch3Puckette(
    const SCPolarBuf *frameNext, 
    const SCPolarBuf *framePrev1,
    const SCPolarBuf *framePrev2, 
    SCPolarBuf *outFrame,
    const SCPolarBuf *outFramePrev,
    float pos,
    size_t fftSize, 
    size_t hopSize);

/// Fills a SCPolarBuf with saved STFT data from a single frame
///
/// \param fftBuf The FFT frame from the STFT buffer
/// \param [out] polarBuf The SCPolarBuf to copy to
/// \param fftSize The FFT size
void fillPolarBuf(const float *fftBuf, SCPolarBuf *polarBuf, size_t fftSize);

/// Copies data from one SCPolarBuf to another
///
/// \param sourceBuf The source buffer
/// \param [out] destBuf The destination buffer
/// \param numbins The number of bins in the SCPolarBuf (fftSize/2-1)
void copyPolarBuf(const SCPolarBuf *sourceBuf, SCPolarBuf *destBuf, size_t numbins);