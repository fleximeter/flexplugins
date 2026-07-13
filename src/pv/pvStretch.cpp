/*
File: pvStretch.cpp
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

#include "pvStretch.hpp"
#include "SC_Constants.h"
#include <iostream>
#include <complex>
#define INTERP(a, b, pos) (a + (b-a) * pos)

extern InterfaceTable *ft;

FlexPlugins::PV_PlayBufStretch::PV_PlayBufStretch() {
    PV_PlayBufStretch *unit = this;
    PV_GET_BUF
    
    // Connect to the STFT buffer. For now, we only allow this in the constructor.
    float fstftbufnum = in0(1);
    uint32 stftbufnum = static_cast<uint32>(fstftbufnum);
    if (stftbufnum >= mWorld->mNumSndBufs) stftbufnum = 0;
    m_fbufnum = fstftbufnum;
    m_buf = mWorld->mSndBufs + stftbufnum;
    m_outFramePrev = nullptr;
    m_frameNext = nullptr;
    m_framePrev1 = nullptr;
    m_framePrev2 = nullptr;
    size_t peakRadius = static_cast<size_t>(sc_clip(in0(6), 1.f, 32.f));
    m_peakFinder = (PeakFinder*)RTAlloc(mWorld, sizeof(PeakFinder));
    new (m_peakFinder) PeakFinder(static_cast<size_t>(buf->samples), peakRadius);
    m_peakFinder->memLoad(RTAlloc(mWorld, m_peakFinder->memSize()));
    
    // Configure position
    float startPos = sc_clip<float>(in0(2), 0.0, 1.0);
    m_pos = 0.f;
    m_startPos = startPos;
    m_firstFrame = true;

    set_calc_function<PV_PlayBufStretch, &PV_PlayBufStretch::next>();
    next(1);
}

FlexPlugins::PV_PlayBufStretch::~PV_PlayBufStretch() {
    if (m_peakFinder) {
        void *reservedMem = m_peakFinder->memRetrieve();
        if (reservedMem) {
            RTFree(mWorld, reservedMem);
        }
        RTFree(mWorld, m_peakFinder);
    }
    if (m_outFramePrev) {
        RTFree(mWorld, m_outFramePrev);
    }
    if (m_frameNext) {
        RTFree(mWorld, m_frameNext);
    }
    if (m_framePrev1) {
        RTFree(mWorld, m_framePrev1);
    }
    if (m_framePrev2) {
        RTFree(mWorld, m_framePrev2);
    }
}

void FlexPlugins::PV_PlayBufStretch::next(int inNumSamples) {
    PV_PlayBufStretch *unit = this;
    PV_GET_BUF
    
    // This section of the code is for acquiring the STFT buffer and information about it.
    // It has to be run every time because we cannot be sure the user has not freed the buffer.
    // We also have to verify important details about the buffer to make sure we can read from it at all.
    if (!m_buf) {
        OUT0(0) = -1.f;
        std::cout << "WARNING: The stftBuffer could not be accessed. Aborting.\n";
        return;
    }
    ACQUIRE_SNDBUF_SHARED(stftBuf);
    const float* bufData __attribute__((__unused__)) = m_buf->data;
    const float* stftData __attribute__((__unused__)) = m_buf->data + 3;
    const uint32 bufChannels __attribute__((__unused__)) = m_buf->channels;
    const uint32 bufSamples __attribute__((__unused__)) = m_buf->samples;
    const uint32 bufFrames = m_buf->frames;
    // first 3 frames have analysis parameters
    const int stftFrames = (static_cast<int>(m_buf->samples) - 3) / static_cast<int>(buf->samples);

    // If the buffer is improperly configured, we cannot use it.
    if (bufChannels != 1) {
        OUT0(0) = -1.f;
        std::cout << "WARNING: stftBuf is configured with " << bufChannels << 
            " channels. A properly formatted buffer must only have 1 channel. Aborting.\n";
        RELEASE_SNDBUF_SHARED(stftBuf);
        return;
    }
    else if (stftFrames < 2) {
        OUT0(0) = -1.f;
        std::cout << "WARNING: stftBuf has " << stftFrames << " frames, which is not enough data to be useful. " << 
            "Note that a properly formatted stftBuf has three leading elements: the FFT size, the hop size, " << 
            "and the window type. After that, it contains M analysis STFT frames, each of size N (FFT size). Aborting.\n";
        RELEASE_SNDBUF_SHARED(stftBuf);
        return;
    }

    // Basic information
    const size_t stftBufFftSize = static_cast<size_t>(bufData[0]);
    const size_t stftBufHopSize = static_cast<size_t>(bufData[1] * stftBufFftSize);  // in frames, not fraction
    const int stftBufWinType = static_cast<int>(bufData[2]);  // -1, 0, or 1. This information is probably extraneous.
    
    if (stftBufFftSize != buf->samples) {
        OUT0(0) = -1.f;
        std::cout << "WARNING: The FFT size of the STFT buffer (" << stftBufFftSize << 
            ") does not match the FFT size of the PV_PlayBufStretch (" << 
            buf->samples << "). Aborting.\n";
        RELEASE_SNDBUF_SHARED(stftBuf);
        return;
    }

    // The first time through, we need to allocate the SCPolarBuf storage in the UGen.
    if (!m_outFramePrev) {
        // This is an annoying way to have to allocate memory,
        // but it seems to be necessary based on how SCPolarBuf is defined.
        float *outFramePrev = (float*)RTAlloc(mWorld, stftBufFftSize * sizeof(float));
        float *frameNext = (float*)RTAlloc(mWorld, stftBufFftSize * sizeof(float));
        float *framePrev1 = (float*)RTAlloc(mWorld, stftBufFftSize * sizeof(float));
        float *framePrev2 = (float*)RTAlloc(mWorld, stftBufFftSize * sizeof(float));
        m_outFramePrev = (SCPolarBuf*)outFramePrev;
        m_frameNext = (SCPolarBuf*)frameNext;
        m_framePrev1 = (SCPolarBuf*)framePrev1;
        m_framePrev2 = (SCPolarBuf*)framePrev2;
    }
    
    float startPos = sc_clip<float>(in0(2), 0.0, 1.0);
    const float rate = in0(3);
    const float loop = in0(4);
    
    if (startPos != m_startPos) {
        m_startPos = startPos;
        m_firstFrame = true;
        // std::cout << "Start pos changed\n";
    }

    // Now that we've run setup, we're ready to read STFT data and perform phase vocoder stretching.

    // First we need to figure out where we are, and if that means we need to loop or quit.
    float newPos = m_pos + rate;
    if (newPos > stftFrames - 1) {
        if (loop && rate > 0) {
            m_firstFrame = true;
            startPos = 0;
        } else if (rate <= 0) {
            // clip it to the last possible frame if we're working backwards
            newPos = stftFrames - 1;
        } else {
            float *outBuf = out(0);
            outBuf[0] = -1.f;
            RELEASE_SNDBUF_SHARED(stftBuf);
            DoneAction(static_cast<int>(in0(7)), this);
            return;
        }
    } else if (newPos < 0) {
        if (loop && rate < 0) {
            m_firstFrame = true;
            startPos = 1;
        } else if (rate >= 0) {
            // clip it to the first frame if we're working forwards
            newPos = 0;
        } else {
            float *outBuf = out(0);
            outBuf[0] = -1.f;
            RELEASE_SNDBUF_SHARED(stftBuf);
            DoneAction(static_cast<int>(in0(7)), this);
            return;
        }
    }

    // The first frame has to be cloned directly from the STFT buffer with no phase adjustments.
    // This is essential to make sure that subsequent phase calculations are correctly aligned.
    if (m_firstFrame) {
        // Compute the index of the first frame
        size_t firstFrameIdx = static_cast<size_t>(std::round(startPos * (stftFrames-1)));
        
        // Copy the FFT data over
        SCPolarBuf *p = ToPolarApx(buf);
        const float *currentFftFrame = stftData + (firstFrameIdx * stftBufFftSize);
        // Fill the output buffer
        fillPolarBuf(currentFftFrame, p, stftBufFftSize);
        
        // We always need to store the resultant FFT frame in the UGen.
        // It is used in the next call to PV_PlayBufStretch_next, for 
        // phase vocoder calculations.
        fillPolarBuf(currentFftFrame, m_outFramePrev, stftBufFftSize);

        // We have to advance by one frame because we need to be able to compute frequency for time stretching.
        m_pos = static_cast<float>(firstFrameIdx + 1);
        m_firstFrame = false;
    }
    
    // For frames other than the first frame, we'll need to perform phase computation.
    else {
        size_t phaseLock = sc_clip(static_cast<size_t>(in0(5)), 0, 2);
        SCPolarBuf *p = ToPolarApx(buf);
        size_t roundedPos = static_cast<size_t>(std::round(newPos));
    
        // If we're right smack on a specific FFT frame, we don't
        // need to do any magnitude or frequency interpolation, so
        // we only need the current and previous FFT frames from the buffer.
        if (std::abs(roundedPos-newPos) < 1e-3) {
            size_t lastPos = rate >= 0 ? roundedPos - 1 : roundedPos + 1;
            if (lastPos < 0) {
                lastPos = roundedPos + 1;
            } else if (lastPos >= stftFrames) {
                lastPos = roundedPos - 1;
            }
            fillPolarBuf(stftData + (roundedPos * stftBufFftSize), m_frameNext, stftBufFftSize);
            fillPolarBuf(stftData + (lastPos * stftBufFftSize), m_framePrev1, stftBufFftSize);
            // Render the output FFT frame
            switch (phaseLock) {
                case 1:
                    Stretch2Puckette(
                        m_frameNext, 
                        m_framePrev1, 
                        p, 
                        m_outFramePrev, 
                        stftBufFftSize, 
                        stftBufHopSize
                    );
                    break;
                case 2:
                    Stretch2LarocheDolson(
                        m_frameNext, 
                        m_framePrev1, 
                        p, 
                        m_outFramePrev, 
                        m_peakFinder,
                        stftBufFftSize, 
                        stftBufHopSize
                    );
                    break;
                default:
                    Stretch2(
                        m_frameNext, 
                        m_framePrev1, 
                        p, 
                        m_outFramePrev, 
                        stftBufFftSize, 
                        stftBufHopSize
                    );
            }
        } else {
            // Otherwise we're between two FFT frames, and we're going to have to
            // interpolate magnitude and frequency data.
            size_t lo, hi, loprev;
            if (rate >= 0) {                
                lo = static_cast<size_t>(std::floor(newPos));
                hi = static_cast<size_t>(std::ceil(newPos));
                loprev = lo == 0 ? 0 : lo - 1;
            } else {
                hi = static_cast<size_t>(std::floor(newPos));
                lo = static_cast<size_t>(std::ceil(newPos));
                loprev = lo + 1;
                if (loprev >= stftFrames) loprev = lo;
            }

            // We are in between these two frames
            fillPolarBuf(stftData + (hi * stftBufFftSize), m_frameNext, stftBufFftSize);
            fillPolarBuf(stftData + (lo * stftBufFftSize), m_framePrev1, stftBufFftSize);
            // This is the frame right before that. It's needed to compute the previous instantaneous frequencies.
            fillPolarBuf(stftData + (loprev * stftBufFftSize), m_framePrev2, stftBufFftSize);
            // Render the output FFT frame
            switch (phaseLock) {
                case 1:
                    Stretch3Puckette(
                        m_frameNext, 
                        m_framePrev1,
                        m_framePrev2, 
                        p, 
                        m_outFramePrev,
                        newPos-static_cast<float>(lo), 
                        stftBufFftSize, 
                        stftBufHopSize
                    );
                    break;
                case 2:
                    Stretch3LarocheDolson(
                        m_frameNext, 
                        m_framePrev1, 
                        m_framePrev2, 
                        p, 
                        m_outFramePrev, 
                        m_peakFinder,
                        newPos-static_cast<float>(lo), 
                        stftBufFftSize, 
                        stftBufHopSize
                    );
                    break;
                default:
                    Stretch3(
                        m_frameNext, 
                        m_framePrev1, 
                        m_framePrev2, 
                        p, 
                        m_outFramePrev, 
                        newPos-static_cast<float>(lo), 
                        stftBufFftSize, 
                        stftBufHopSize
                    );
            }
        }
        // We always need to store the resultant FFT frame in the UGen.
        // It is used in the next call to PV_PlayBufStretch_next, for 
        // phase vocoder calculations.
        copyPolarBuf(p, m_outFramePrev, static_cast<size_t>(numbins));
        m_pos = newPos;
    }

    RELEASE_SNDBUF_SHARED(stftBuf);
}

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
void FlexPlugins::PV_PlayBufStretch::Stretch2(
    const SCPolarBuf *frame, 
    const SCPolarBuf *framePrev, 
    SCPolarBuf *outFrame, 
    const SCPolarBuf *outFramePrev, 
    size_t fftSize, 
    size_t hopSize) {
    outFrame->dc = frame->dc;
    outFrame->nyq = frame->nyq;
    for (size_t xxk = 0; xxk < fftSize/2-1; xxk++) {
        outFrame->bin[xxk].mag = frame->bin[xxk].mag;

        // Compute the instantaneous frequency
        float omegaK = twopi * (xxk+1) / fftSize;
        float phaseInc = frame->bin[xxk].phase - framePrev->bin[xxk].phase - hopSize * omegaK;
        phaseInc = std::fmod(phaseInc + pi, twopi) - pi;
        float instantaneousFreq = omegaK + phaseInc/hopSize;
        
        // Compute the new phase
        outFrame->bin[xxk].phase = sc_wrap(
            outFramePrev->bin[xxk].phase + static_cast<float>(hopSize * instantaneousFreq),
            0.f,
            static_cast<float>(twopi));
    }
}

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
void FlexPlugins::PV_PlayBufStretch::Stretch2Puckette(
    const SCPolarBuf *frame, 
    const SCPolarBuf *framePrev, 
    SCPolarBuf *outFrame, 
    const SCPolarBuf *outFramePrev, 
    size_t fftSize, 
    size_t hopSize) {
    outFrame->dc = frame->dc;
    outFrame->nyq = frame->nyq;
    for (size_t xxk = 0; xxk < fftSize/2-1; xxk++) {
        outFrame->bin[xxk].mag = frame->bin[xxk].mag;

        // Compute the instantaneous frequency
        float omegaK = twopi * (xxk+1) / fftSize;
        float phaseInc = frame->bin[xxk].phase - framePrev->bin[xxk].phase - hopSize * omegaK;
        phaseInc = std::fmod(phaseInc + pi, twopi) - pi;
        float instantaneousFreq = omegaK + phaseInc/hopSize;

        // In Puckette-style phase locking, we make a substitution for the previous phase,
        // in order to "lock" phases of adjacent bins together.
        float prevPhase = 0.0;
        if (xxk == 0) {
            prevPhase = outFramePrev->bin[xxk].phase;
        } else if (xxk == fftSize/2-2) {
            prevPhase = outFramePrev->bin[xxk].phase;
        } else {
            std::complex<float> prevBinKMinus1 = std::polar<float>(outFramePrev->bin[xxk-1].mag, outFramePrev->bin[xxk-1].phase);
            std::complex<float> prevBinK = std::polar<float>(outFramePrev->bin[xxk].mag, outFramePrev->bin[xxk].phase);
            std::complex<float> prevBinKPlus1 = std::polar<float>(outFramePrev->bin[xxk+1].mag, outFramePrev->bin[xxk+1].phase);
            prevPhase = std::arg(prevBinK - prevBinKMinus1 - prevBinKPlus1);
        }
        
        // Compute the new phase
        outFrame->bin[xxk].phase = sc_wrap(
            prevPhase + static_cast<float>(hopSize * instantaneousFreq),
            0.f,
            static_cast<float>(twopi));
    }
}

/// Computes a single frame of STFT data for time stretching,
/// using the Laroche/Dolson identity phase locking scheme.
/// The assumption is that we are positioned exactly at `frame`, and we therefore
/// just need framePrev to compute the instantaneous frequency. We also do not
/// need to perform any magnitude or frequency interpolation.
///
/// \param frame The current STFT frame
/// \param framePrev The previous STFT frame
/// \param [out] outFrame The output STFT frame
/// \param outFramePrev The previously computed output STFT frame
/// \param peakFinder The PeakFinder instance for determining peak locations in the magnitude spectrum
/// \param fftSize The FFT size
/// \param hopSize The hop size
void FlexPlugins::PV_PlayBufStretch::Stretch2LarocheDolson(
    const SCPolarBuf *frame, 
    const SCPolarBuf *framePrev, 
    SCPolarBuf *outFrame, 
    const SCPolarBuf *outFramePrev, 
    PeakFinder *peakFinder,
    size_t fftSize, 
    size_t hopSize) {
    outFrame->dc = frame->dc;
    outFrame->nyq = frame->nyq;

    // Acquire peaks
    peakFinder->analyze(frame);

    if (peakFinder->size() == 0) {
        // No phase locking if no peaks were acquired
        for (size_t xxk = 0; xxk < fftSize/2-1; xxk++) {
            outFrame->bin[xxk].mag = frame->bin[xxk].mag;

            // Compute the instantaneous frequency
            double omegaK = twopi * (xxk+1) / fftSize;
            double phaseInc = frame->bin[xxk].phase - framePrev->bin[xxk].phase - hopSize * omegaK;
            phaseInc = std::fmod(phaseInc + pi, twopi) - pi;
            double instantaneousFreq = omegaK + phaseInc/hopSize;

            // Compute the phase
            outFrame->bin[xxk].phase = sc_wrap(
                outFramePrev->bin[xxk].phase + static_cast<float>(hopSize * instantaneousFreq),
                0.f,
                static_cast<float>(twopi));
        }
    } else {
        // Update any bins that occur below the lowest peak's region of influence
        for (size_t xxk = 0; xxk < peakFinder->peaks[0].leftValley; xxk++) {
            outFrame->bin[xxk].mag = frame->bin[xxk].mag;

            // Compute the instantaneous frequency
            double omegaK = twopi * (xxk+1) / fftSize;
            double phaseInc = frame->bin[xxk].phase - framePrev->bin[xxk].phase - hopSize * omegaK;
            phaseInc = std::fmod(phaseInc + pi, twopi) - pi;
            double instantaneousFreq = omegaK + phaseInc/hopSize;

            // Compute the phase
            outFrame->bin[xxk].phase = sc_wrap(
                outFramePrev->bin[xxk].phase + static_cast<float>(hopSize * instantaneousFreq),
                0.f,
                static_cast<float>(twopi));
        }

        // Update any bins that occur above the highest peak's region of influence
        for (size_t xxk = peakFinder->peaks[peakFinder->size()-1].rightValley + 1; xxk < fftSize / 2 - 1; xxk++) {
            outFrame->bin[xxk].mag = frame->bin[xxk].mag;

            // Compute the instantaneous frequency
            double omegaK = twopi * (xxk+1) / fftSize;
            double phaseInc = frame->bin[xxk].phase - framePrev->bin[xxk].phase - hopSize * omegaK;
            phaseInc = std::fmod(phaseInc + pi, twopi) - pi;
            double instantaneousFreq = omegaK + phaseInc/hopSize;

            // Compute the phase
            outFrame->bin[xxk].phase = sc_wrap(
                outFramePrev->bin[xxk].phase + static_cast<float>(hopSize * instantaneousFreq),
                0.f,
                static_cast<float>(twopi));
        }

        // Update all other peaks
        for (size_t xxn = 0; xxn < peakFinder->size(); xxn++) {
            // First we compute the new phase for the peak bin as usual
            Peak peak = peakFinder->peaks[xxn];
            outFrame->bin[peak.peak].mag = frame->bin[peak.peak].mag;

            // Compute the instantaneous frequency
            double omegaK = twopi * (peak.peak+1) / fftSize;
            double phaseInc = frame->bin[peak.peak].phase - framePrev->bin[peak.peak].phase - hopSize * omegaK;
            phaseInc = std::fmod(phaseInc + pi, twopi) - pi;
            double instantaneousFreq = omegaK + phaseInc/hopSize;

            // Compute the phase
            outFrame->bin[peak.peak].phase = sc_wrap(
                outFramePrev->bin[peak.peak].phase + static_cast<float>(hopSize * instantaneousFreq),
                0.f,
                static_cast<float>(twopi));

            // Then we update the phases of all other peaks
            for (size_t xxo = peak.leftValley; xxo < peak.peak; xxo++) {
                outFrame->bin[xxo].mag = frame->bin[xxo].mag;
                outFrame->bin[xxo].phase = sc_wrap(
                    outFrame->bin[peak.peak].phase + frame->bin[xxo].phase - frame->bin[peak.peak].phase,
                    0.f,
                    static_cast<float>(twopi));
            }
            for (size_t xxo = peak.peak + 1; xxo <= peak.rightValley; xxo++) {
                outFrame->bin[xxo].mag = frame->bin[xxo].mag;
                outFrame->bin[xxo].phase = sc_wrap(
                    outFrame->bin[peak.peak].phase + frame->bin[xxo].phase - frame->bin[peak.peak].phase,
                    0.f,
                    static_cast<float>(twopi));
            }
        }
    }
}

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
void FlexPlugins::PV_PlayBufStretch::Stretch3(
    const SCPolarBuf *frameNext, 
    const SCPolarBuf *framePrev1,
    const SCPolarBuf *framePrev2, 
    SCPolarBuf *outFrame,
    const SCPolarBuf *outFramePrev,
    float pos,
    size_t fftSize, 
    size_t hopSize) {
    outFrame->dc = INTERP(framePrev1->dc, frameNext->dc, pos);
    outFrame->nyq = INTERP(framePrev1->nyq, frameNext->nyq, pos);
    for (size_t xxk = 0; xxk < fftSize/2-1; xxk++) {
        outFrame->bin[xxk].mag = INTERP(framePrev1->bin[xxk].mag, frameNext->bin[xxk].mag, pos);

        float omegaK = twopi * (xxk+1) / fftSize;

        // Compute the next instantaneous frequency
        float phaseInc = frameNext->bin[xxk].phase - framePrev1->bin[xxk].phase - hopSize * omegaK;
        phaseInc = std::fmod(phaseInc + pi, twopi) - pi;
        float instantaneousFreqNext = omegaK + phaseInc/hopSize;

        // Compute the previous instantaneous frequency
        phaseInc = framePrev1->bin[xxk].phase - framePrev2->bin[xxk].phase - hopSize * omegaK;
        phaseInc = std::fmod(phaseInc + pi, twopi) - pi;
        float instantaneousFreqPrev = omegaK + phaseInc/hopSize;

        // Interpolate the instantaneous frequency
        float instantaneousFreq = INTERP(instantaneousFreqPrev, instantaneousFreqNext, pos);
        
        // Compute the new phase
        outFrame->bin[xxk].phase = sc_wrap(
            outFramePrev->bin[xxk].phase + static_cast<float>(hopSize * instantaneousFreq),
            0.f,
            static_cast<float>(twopi));
    }
}

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
void FlexPlugins::PV_PlayBufStretch::Stretch3Puckette(
    const SCPolarBuf *frameNext, 
    const SCPolarBuf *framePrev1,
    const SCPolarBuf *framePrev2, 
    SCPolarBuf *outFrame,
    const SCPolarBuf *outFramePrev,
    float pos,
    size_t fftSize, 
    size_t hopSize) {
    outFrame->dc = INTERP(framePrev1->dc, frameNext->dc, pos);
    outFrame->nyq = INTERP(framePrev1->nyq, frameNext->nyq, pos);
    for (size_t xxk = 0; xxk < fftSize/2-1; xxk++) {
        outFrame->bin[xxk].mag = INTERP(framePrev1->bin[xxk].mag, frameNext->bin[xxk].mag, pos);

        float omegaK = twopi * (xxk+1) / fftSize;

        // Compute the next instantaneous frequency
        float phaseInc = frameNext->bin[xxk].phase - framePrev1->bin[xxk].phase - hopSize * omegaK;
        phaseInc = std::fmod(phaseInc + pi, twopi) - pi;
        float instantaneousFreqNext = omegaK + phaseInc/hopSize;

        // Compute the previous instantaneous frequency
        phaseInc = framePrev1->bin[xxk].phase - framePrev2->bin[xxk].phase - hopSize * omegaK;
        phaseInc = std::fmod(phaseInc + pi, twopi) - pi;
        float instantaneousFreqPrev = omegaK + phaseInc/hopSize;

        // Interpolate the instantaneous frequency
        float instantaneousFreq = INTERP(instantaneousFreqPrev, instantaneousFreqNext, pos);

        // In Puckette-style phase locking, we make a substitution for the previous phase,
        // in order to "lock" phases of adjacent bins together.
        float prevPhase = 0.0;
        if (xxk == 0) {
            prevPhase = outFramePrev->bin[xxk].phase;
        } else if (xxk == fftSize/2-2) {
            prevPhase = outFramePrev->bin[xxk].phase;
        } else {
            std::complex<float> prevBinKMinus1 = std::polar<float>(outFramePrev->bin[xxk-1].mag, outFramePrev->bin[xxk-1].phase);
            std::complex<float> prevBinK = std::polar<float>(outFramePrev->bin[xxk].mag, outFramePrev->bin[xxk].phase);
            std::complex<float> prevBinKPlus1 = std::polar<float>(outFramePrev->bin[xxk+1].mag, outFramePrev->bin[xxk+1].phase);
            prevPhase = std::arg(prevBinK - prevBinKMinus1 - prevBinKPlus1);
        }
        
        // Compute the new phase
        outFrame->bin[xxk].phase = sc_wrap(
            prevPhase + static_cast<float>(hopSize * instantaneousFreq),
            0.f,
            static_cast<float>(twopi));
    }
}

/// Computes a single frame of STFT data for time stretching,
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
/// \param peakFinder The PeakFinder instance for determining peak locations in the magnitude spectrum
/// \param pos The position between framePrev1 and frameNext (0 < pos < 1)
/// \param fftSize The FFT size
/// \param hopSize The hop size
void FlexPlugins::PV_PlayBufStretch::Stretch3LarocheDolson(
    const SCPolarBuf *frameNext,
    const SCPolarBuf *framePrev1, 
    const SCPolarBuf *framePrev2, 
    SCPolarBuf *outFrame, 
    const SCPolarBuf *outFramePrev,
    PeakFinder *peakFinder,
    double pos,
    size_t fftSize, 
    size_t hopSize) {
    outFrame->dc = INTERP(framePrev1->dc, frameNext->dc, pos);
    outFrame->nyq = INTERP(framePrev1->nyq, frameNext->nyq, pos);
    
    // Get the closest bin for phase relationships
    const SCPolarBuf *cframe = nullptr;
    if (pos >= 0.5) {
        cframe = frameNext;
    } else {
        cframe = framePrev1;
    }

    // Acquire peaks
    peakFinder->analyze(cframe);

    if (peakFinder->size() == 0) {
        // No phase locking if no peaks were acquired
        for (size_t xxk = 0; xxk < fftSize/2-1; xxk++) {
            outFrame->bin[xxk].mag = INTERP(framePrev1->bin[xxk].mag, frameNext->bin[xxk].mag, pos);
            
            double omegaK = twopi * (xxk+1) / fftSize;

            // Compute the next instantaneous frequency
            double phaseInc = frameNext->bin[xxk].phase - framePrev1->bin[xxk].phase - hopSize * omegaK;
            phaseInc = std::fmod(phaseInc + pi, twopi) - pi;
            double instantaneousFreqNext = omegaK + phaseInc/hopSize;

            // Compute the previous instantaneous frequency
            phaseInc = framePrev1->bin[xxk].phase - framePrev2->bin[xxk].phase - hopSize * omegaK;
            phaseInc = std::fmod(phaseInc + pi, twopi) - pi;
            double instantaneousFreqPrev = omegaK + phaseInc/hopSize;

            // Interpolate the instantaneous frequency
            double instantaneousFreq = INTERP(instantaneousFreqPrev, instantaneousFreqNext, pos);
            
            // Compute the new phase
            outFrame->bin[xxk].phase = sc_wrap(
                outFramePrev->bin[xxk].phase + static_cast<float>(hopSize * instantaneousFreq),
                0.f,
                static_cast<float>(twopi));
        }
    } else {
        // Update any bins that occur below the lowest peak's region of influence
        for (size_t xxk = 0; xxk < peakFinder->peaks[0].leftValley; xxk++) {
            outFrame->bin[xxk].mag = INTERP(framePrev1->bin[xxk].mag, frameNext->bin[xxk].mag, pos);

            // Compute the instantaneous frequency
            double omegaK = twopi * (xxk+1) / fftSize;

            double phaseInc = frameNext->bin[xxk].phase - framePrev1->bin[xxk].phase - hopSize * omegaK;
            phaseInc = std::fmod(phaseInc + pi, twopi) - pi;
            double instantaneousFreqNext = omegaK + phaseInc/hopSize;

            // Compute the previous instantaneous frequency
            phaseInc = framePrev1->bin[xxk].phase - framePrev2->bin[xxk].phase - hopSize * omegaK;
            phaseInc = std::fmod(phaseInc + pi, twopi) - pi;
            double instantaneousFreqPrev = omegaK + phaseInc/hopSize;

            // Interpolate the instantaneous frequency
            double instantaneousFreq = INTERP(instantaneousFreqPrev, instantaneousFreqNext, pos);

            // Compute the phase
            outFrame->bin[xxk].phase = sc_wrap(
                outFramePrev->bin[xxk].phase + static_cast<float>(hopSize * instantaneousFreq),
                0.f,
                static_cast<float>(twopi));
        }

        // Update any bins that occur above the highest peak's region of influence
        for (size_t xxk = peakFinder->peaks[peakFinder->size()-1].rightValley + 1; xxk < fftSize / 2 - 1; xxk++) {
            outFrame->bin[xxk].mag = INTERP(framePrev1->bin[xxk].mag, frameNext->bin[xxk].mag, pos);

            // Compute the instantaneous frequency
            double omegaK = twopi * (xxk+1) / fftSize;

            double phaseInc = frameNext->bin[xxk].phase - framePrev1->bin[xxk].phase - hopSize * omegaK;
            phaseInc = std::fmod(phaseInc + pi, twopi) - pi;
            double instantaneousFreqNext = omegaK + phaseInc/hopSize;

            // Compute the previous instantaneous frequency
            phaseInc = framePrev1->bin[xxk].phase - framePrev2->bin[xxk].phase - hopSize * omegaK;
            phaseInc = std::fmod(phaseInc + pi, twopi) - pi;
            double instantaneousFreqPrev = omegaK + phaseInc/hopSize;

            // Interpolate the instantaneous frequency
            double instantaneousFreq = INTERP(instantaneousFreqPrev, instantaneousFreqNext, pos);

            // Compute the phase
            outFrame->bin[xxk].phase = sc_wrap(
                outFramePrev->bin[xxk].phase + static_cast<float>(hopSize * instantaneousFreq),
                0.f,
                static_cast<float>(twopi));
        }
        
        for (size_t xxn = 0; xxn < peakFinder->size(); xxn++) {
            // First we compute the new phase for the peak bin as usual
            Peak peak = peakFinder->peaks[xxn];
            outFrame->bin[peak.peak].mag = INTERP(framePrev1->bin[peak.peak].mag, frameNext->bin[peak.peak].mag, pos);
            
            // Compute the instantaneous frequency
            double omegaK = twopi * (peak.peak+1) / fftSize;

            double phaseInc = frameNext->bin[peak.peak].phase - framePrev1->bin[peak.peak].phase - hopSize * omegaK;
            phaseInc = std::fmod(phaseInc + pi, twopi) - pi;
            double instantaneousFreqNext = omegaK + phaseInc/hopSize;

            // Compute the previous instantaneous frequency
            phaseInc = framePrev1->bin[peak.peak].phase - framePrev2->bin[peak.peak].phase - hopSize * omegaK;
            phaseInc = std::fmod(phaseInc + pi, twopi) - pi;
            double instantaneousFreqPrev = omegaK + phaseInc/hopSize;

            // Interpolate the instantaneous frequency
            double instantaneousFreq = INTERP(instantaneousFreqPrev, instantaneousFreqNext, pos);

            // Compute the phase
            outFrame->bin[peak.peak].phase = sc_wrap(
                outFramePrev->bin[peak.peak].phase + static_cast<float>(hopSize * instantaneousFreq),
                0.f,
                static_cast<float>(twopi));

            // Then we update all bins in the region of influence
            for (size_t xxo = peak.leftValley; xxo < peak.peak; xxo++) {
                outFrame->bin[xxo].mag = INTERP(framePrev1->bin[xxo].mag, frameNext->bin[xxo].mag, pos);
                outFrame->bin[xxo].phase = sc_wrap(
                    outFrame->bin[peak.peak].phase + cframe->bin[xxo].phase - cframe->bin[peak.peak].phase,
                    0.f,
                    static_cast<float>(twopi));
            }
            for (size_t xxo = peak.peak + 1; xxo <= peak.rightValley; xxo++) {
                outFrame->bin[xxo].mag = INTERP(framePrev1->bin[xxo].mag, frameNext->bin[xxo].mag, pos);
                outFrame->bin[xxo].phase = sc_wrap(
                    outFrame->bin[peak.peak].phase + cframe->bin[xxo].phase - cframe->bin[peak.peak].phase,
                    0.f,
                    static_cast<float>(twopi));
            }
        }
    }
}

/// Fills a SCPolarBuf with saved STFT data from a single frame
///
/// \param fftBuf The FFT frame from the STFT buffer
/// \param [out] polarBuf The SCPolarBuf to copy to
/// \param fftSize The FFT size
void FlexPlugins::PV_PlayBufStretch::fillPolarBuf(const float *fftBuf, SCPolarBuf *polarBuf, size_t fftSize) {
    polarBuf->dc = fftBuf[0];
    polarBuf->nyq = fftBuf[1];
    for (size_t xxn = 2, xxk = 0; xxn < fftSize; xxn+=2, xxk++) {
        // For some reason the phase is stored first, then the magnitude.
        // This prevents a direct cast to SCPolarBuf, unfortunately.
        polarBuf->bin[xxk].phase = fftBuf[xxn];
        polarBuf->bin[xxk].mag = fftBuf[xxn+1];
    }
}

/// Copies data from one SCPolarBuf to another
///
/// \param sourceBuf The source buffer
/// \param [out] destBuf The destination buffer
/// \param numbins The number of bins in the SCPolarBuf (fftSize/2-1)
void FlexPlugins::PV_PlayBufStretch::copyPolarBuf(const SCPolarBuf *sourceBuf, SCPolarBuf *destBuf, size_t numbins) {
    destBuf->dc = sourceBuf->dc;
    destBuf->nyq = sourceBuf->nyq;
    for (size_t xxn = 0; xxn < numbins; xxn++) {
        destBuf->bin[xxn].mag = sourceBuf->bin[xxn].mag;
        destBuf->bin[xxn].phase = sourceBuf->bin[xxn].phase;
    }
}

FlexPlugins::Peak::Peak(size_t peak) : peak(peak) {}
FlexPlugins::Peak::Peak(size_t peak, size_t leftValley, size_t rightValley) : peak(peak), leftValley(leftValley), rightValley(rightValley) {}

FlexPlugins::PeakFinder::PeakFinder(size_t fftSize, size_t radius) {
    m_maxSize = fftSize/2-1;
    m_radius = radius;
    m_size = 0;
    peaks = nullptr;
    m_queueL = nullptr;
    m_queueR = nullptr;
}

void FlexPlugins::PeakFinder::memLoad(void* arr) {
    size_t *data = (size_t*)arr;
    m_queueL = data;
    m_queueR = data + m_radius;
    peaks = (Peak*)(data + 2 * m_radius);
}

size_t FlexPlugins::PeakFinder::memSize() const {
    return m_radius * 2 * sizeof(size_t) + m_maxSize * sizeof(Peak);
}

void* FlexPlugins::PeakFinder::memRetrieve() {
    return (void*)m_queueL;
}

void FlexPlugins::PeakFinder::clear() {
    m_size = 0;
}

size_t FlexPlugins::PeakFinder::maxSize() const {
    return m_maxSize;
}

size_t FlexPlugins::PeakFinder::size() const {
    return m_size;
}

void FlexPlugins::PeakFinder::analyze(const SCPolarBuf *buf) {
    // We can only perform the analysis if we have enough bins
    if (m_queueL && m_maxSize > m_radius * 2 + 1) {
        m_size = 0;  // clear any existing data
        
        size_t xxi = m_radius;
        while (xxi < m_maxSize - m_radius) {
            bool isMax = true;
            for (size_t xxj = xxi - m_radius; xxj < xxi; xxj++) {
                if (buf->bin[xxj].mag >= buf->bin[xxi].mag) {
                    isMax = false;
                    break;
                }
            }
            for (size_t xxj = xxi + 1; xxj <= xxi + m_radius; xxj++) {
                if (buf->bin[xxj].mag >= buf->bin[xxi].mag) {
                    isMax = false;
                    break;
                }
            }
            if (isMax) {
                peaks[m_size] = Peak(xxi);
                m_size++;
                xxi += m_radius + 1;
            } else {
                xxi++;
            }
        }

        // Find the left valley for the first peak, and the right valley
        // for the last peak.
        if (m_size > 0) {
            float min = buf->bin[0].mag;
            size_t argmin = 0;
            size_t xxk = 1;
            for (; xxk < peaks[0].peak; xxk++) {
                if (buf->bin[xxk].mag < min) {
                    min = buf->bin[xxk].mag;
                    argmin = xxk;
                }
            }
            peaks[0].leftValley = argmin;
            xxk = peaks[m_size-1].peak + 1;
            min = buf->bin[xxk].mag;
            argmin = xxk;
            for (xxk++; xxk < m_maxSize; xxk++) {
                if (buf->bin[xxk].mag < min) {
                    min = buf->bin[xxk].mag;
                    argmin = xxk;
                }
            }
            peaks[m_size-1].rightValley = argmin;
        } 
        
        // Find the remaining left and right valleys
        if (m_size > 1) {
            for (size_t xxj = 0; xxj < m_size-1; xxj++) {                
                size_t xxk = peaks[xxj].peak + 1;
                float min = buf->bin[xxk].mag;
                size_t argmin = xxk;
                for (xxk++; xxk < peaks[xxj+1].peak; xxk++) {
                    if (buf->bin[xxk].mag < min) {
                        min = buf->bin[xxk].mag;
                        argmin = xxk;
                    }
                }
                peaks[xxj].rightValley = argmin - 1;
                peaks[xxj+1].leftValley = argmin;
            }
        }
    }
}
