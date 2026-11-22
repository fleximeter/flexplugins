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
#include "SC_PlugIn.hpp"
#include "FFT_UGens.h"
#include "peakFinder.hpp"
#include "hashtable.hpp"
#include "pqueue.hpp"
#include "utilities.hpp"

namespace FlexPlugins {
    /// Computes the centered frequency gradient vector for a FFT frame
    ///
    /// \param currentFrame The current FFT frame in the STFT matrix
    /// \param [out] freqGradient The output frequency gradient vector
    /// \param fftSize The FFT size
    /// \param signalLen The total length of the signal in samples
    void cDiffFreq(
        const SCPolarBuf *currentFrame, 
        float *freqGradient, 
        const size_t fftSize, 
        const double signalLen
    );

    /// Computes the centered time phase gradient vector
    ///
    /// \param currentFrame The current FFT frame
    /// \param prevFrame The previous FFT frame
    /// \param nextFrame The next FFT frame
    /// \param [out] timeGradient The output time gradient vector
    /// \param fftSize The FFT size
    /// \param hopSize The analysis hop size
    void cDiffTime(
        const SCPolarBuf *currentFrame, 
        const SCPolarBuf *prevFrame, 
        const SCPolarBuf *nextFrame, 
        float *timeGradient, 
        const size_t fftSize, 
        const size_t hopSize
    );

    /// Copies data from one SCPolarBuf to another
    ///
    /// \param sourceBuf The source buffer
    /// \param [out] destBuf The destination buffer
    /// \param numbins The number of bins in the SCPolarBuf (fftSize/2-1)
    void copyPolarBuf(const SCPolarBuf *sourceBuf, SCPolarBuf *destBuf, size_t numbins);

    /// Fills a SCPolarBuf with saved STFT data from a single frame
    ///
    /// \param fftBuf The FFT frame from the STFT buffer
    /// \param [out] polarBuf The SCPolarBuf to copy to
    /// \param fftSize The FFT size
    void fillPolarBuf(const float *fftBuf, SCPolarBuf *polarBuf, size_t fftSize);

    /// Linearly interpolates the magnitude of two STFT frames
    /// 
    /// \param prevFrame The previous STFT frame
    /// \param nextFrame The next STFT frame
    /// \param [out] out The output STFT frame
    /// \param fftSize The FFT size
    /// \param pos The position between the two frames (0 <= pos <= 1)
    void magLinearInterp(
        const SCPolarBuf *prevFrame, 
        const SCPolarBuf *nextFrame, 
        SCPolarBuf *out, 
        const size_t fftSize, 
        const double pos
    );
    
    /// Linearly interpolates the magnitude of two STFT frames using the power
    /// 
    /// \param prevFrame The previous STFT frame
    /// \param nextFrame The next STFT frame
    /// \param [out] out The output STFT frame
    /// \param fftSize The FFT size
    /// \param pos The position between the two frames (0 <= pos <= 1)
    void powerLinearInterp(
        const SCPolarBuf *prevFrame, 
        const SCPolarBuf *nextFrame, 
        SCPolarBuf *out, 
        const size_t fftSize, 
        const double pos
    );

    /// Stores the state of a PV_PlayBufStretch UGen instance
    class PV_PlayBufStretch : public SCUnit {
    public:
        PV_PlayBufStretch();
        ~PV_PlayBufStretch();

    private:
        void next(int inNumSamples);

        /// Computes a single frame of STFT phases for time stretching.
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
            const size_t fftSize, 
            const size_t hopSize
        );

        /// Computes a single frame of STFT phases for time stretching.
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
            const size_t fftSize, 
            const size_t hopSize
        );

        /// Computes a single frame of STFT phases for time stretching,
        /// using the Laroche/Dolson identity phase locking scheme.
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
        void Stretch2LarocheDolson(
            const SCPolarBuf *frame, 
            const SCPolarBuf *framePrev, 
            SCPolarBuf *outFrame, 
            const SCPolarBuf *outFramePrev, 
            const size_t fftSize, 
            const size_t hopSize
        );

        /// Computes a single frame of STFT phases for time stretching, 
        /// using the RTPGHI algorithm.
        ///
        /// \param currentFrame The current STFT frame
        /// \param prevFrame The previous STFT frame
        /// \param cTimeDiff The current time gradient vector
        /// \param pTimeDiff The previous time gradient vector
        /// \param cFreqDiff The current frequency gradient vector
        /// \param [out] outFrame The frame to write output STFT data to
        /// \param outFramePrev The previous output frame
        /// \param alpha The stretch factor
        /// \param tol The magnitude tolerance for phase calculation (0 < tol < 1)
        /// \param fftSize The FFT size
        /// \param hopSize The hop size
        /// \param signalLen The signal length
        /// \param pgHeap The phase gradient heap
        /// \param setI The bin tracker 
        void Stretch2_RTPGHI(
            const SCPolarBuf *currentFrame, 
            const SCPolarBuf *prevFrame, 
            const float *cTimeDiff, 
            const float *pTimeDiff,
            const float *cFreqDiff, 
            SCPolarBuf *outFrame, 
            const SCPolarBuf *outFramePrev, 
            const float alpha,
            const double tol, 
            const size_t fftSize, 
            const size_t hopSize,
            const double signalLen
        );

        /// Computes a single frame of STFT phases for time stretching.
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
            const float pos,
            const size_t fftSize, 
            const size_t hopSize
        );

        /// Computes a single frame of STFT phases for time stretching.
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
            const float pos,
            const size_t fftSize, 
            const size_t hopSize
        );

        /// Computes a single frame of STFT phases for time stretching,
        /// using the Laroche/Dolson identity phase locking scheme.
        /// The assumption is that we are positioned exactly at `frame`, and we therefore
        /// just need framePrev to compute the instantaneous frequency. We also do not
        /// need to perform any magnitude or frequency interpolation.
        ///
        /// \param frameNext The next STFT frame
        /// \param framePrev1 The previous STFT frame
        /// \param framePrev2 The previous STFT frame before that (required for instantaneous frequency interpolation)
        /// \param [out] outFrame The output STFT frame
        /// \param outFramePrev The previously computed output STFT frame
        /// \param pos The position between framePrev1 and frameNext (0 < pos < 1)
        /// \param fftSize The FFT size
        /// \param hopSize The hop size
        void Stretch3LarocheDolson(
            const SCPolarBuf *frameNext,
            const SCPolarBuf *framePrev1, 
            const SCPolarBuf *framePrev2, 
            SCPolarBuf *outFrame, 
            const SCPolarBuf *outFramePrev,
            const double pos,
            const size_t fftSize, 
            const size_t hopSize
        );

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

        /// The frequency bin tracker for the RTPGHI algorithm
        IntHashSet *m_setI;

        /// The heap for the RTPGHI algorithm
        MaxPQueue<float, std::pair<size_t, int>> *m_pgHeap;

        /// Gradient vectors for the RTPGHI algorithm
        float *m_freqGradient;
        float *m_cTimeGradient;
        float *m_pTimeGradient;

        /// Between 0 and 1; represents the start position of playback.
        /// If it jumps during playback, playback will be restarted
        /// at the new m_startPos.
        float m_startPos;
        
        /// A fractional STFT frame index. Unlike m_startPos, it corresponds to the 
        /// integer index of the current STFT frame (from 0 to M-1).
        /// Used for interpolating position.
        float m_pos;
    };
}
