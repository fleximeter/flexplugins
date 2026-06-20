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

void PV_PlayBufStretch_Ctor(PV_PlayBufStretch *unit) {
    PV_GET_BUF
    
    // Connect to the STFT buffer. For now, we only allow this in the constructor.
    float fstftbufnum = IN0(1);
    uint32 stftbufnum = static_cast<uint32>(fstftbufnum);
    if (stftbufnum >= unit->mWorld->mNumSndBufs) stftbufnum = 0;
    unit->m_fbufnum = fstftbufnum;
    unit->m_buf = unit->mWorld->mSndBufs + stftbufnum;
    unit->m_outFramePrev = nullptr;
    unit->m_frameNext = nullptr;
    unit->m_framePrev1 = nullptr;
    unit->m_framePrev2 = nullptr;
    unit->m_peakFinder = (PeakFinder*)RTAlloc(unit->mWorld, sizeof(PeakFinder));
    new (unit->m_peakFinder) PeakFinder(static_cast<size_t>(buf->samples), 2);
    unit->m_peakFinder->memLoad(RTAlloc(unit->mWorld, unit->m_peakFinder->memSize()));
    
    // Configure position
    float startPos = sc_clip<float>(IN0(2), 0.0, 1.0);
    unit->m_pos = 0.f;
    unit->m_startPos = startPos;
    unit->m_firstFrame = true;

    SETCALC(PV_PlayBufStretch_next);
    OUT0(0) = IN0(0);
}

void PV_PlayBufStretch_Dtor(PV_PlayBufStretch *unit) {
    if (unit->m_peakFinder) {
        void *reservedMem = unit->m_peakFinder->memRetrieve();
        if (reservedMem) {
            RTFree(unit->mWorld, reservedMem);
        }
        RTFree(unit->mWorld, unit->m_peakFinder);
    }
    if (unit->m_outFramePrev) {
        RTFree(unit->mWorld, unit->m_outFramePrev);
    }
    if (unit->m_frameNext) {
        RTFree(unit->mWorld, unit->m_frameNext);
    }
    if (unit->m_framePrev1) {
        RTFree(unit->mWorld, unit->m_framePrev1);
    }
    if (unit->m_framePrev2) {
        RTFree(unit->mWorld, unit->m_framePrev2);
    }
}

void PV_PlayBufStretch_next(PV_PlayBufStretch *unit, int inNumSamples) {
    PV_GET_BUF
    
    // This section of the code is for acquiring the STFT buffer and information about it.
    // It has to be run every time because we cannot be sure the user has not freed the buffer.
    // We also have to verify important details about the buffer to make sure we can read from it at all.
    const SndBuf *stftBuf = unit->m_buf;
    if (!stftBuf) {
        OUT0(0) = -1.f;
        std::cout << "WARNING: The stftBuffer could not be accessed. Aborting.\n";
        return;
    }
    ACQUIRE_SNDBUF_SHARED(stftBuf);
    const float* bufData __attribute__((__unused__)) = stftBuf->data;
    const float* stftData __attribute__((__unused__)) = stftBuf->data + 3;
    const uint32 bufChannels __attribute__((__unused__)) = stftBuf->channels;
    const uint32 bufSamples __attribute__((__unused__)) = stftBuf->samples;
    const uint32 bufFrames = stftBuf->frames;
    // first 3 frames have analysis parameters
    const int stftFrames = (static_cast<int>(stftBuf->samples) - 3) / static_cast<int>(buf->samples);

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
    //std::cout << "FFT size: " << stftBufFftSize << " Hop size: " << stftBufHopSize << " Win type: " << stftBufWinType << " STFT frames " << stftFrames << "\n";

    if (stftBufFftSize != buf->samples) {
        OUT0(0) = -1.f;
        std::cout << "WARNING: The FFT size of the STFT buffer (" << stftBufFftSize << 
            ") does not match the FFT size of the PV_PlayBufStretch (" << 
            buf->samples << "). Aborting.\n";
        RELEASE_SNDBUF_SHARED(stftBuf);
        return;
    }

    // The first time through, we need to allocate the SCPolarBuf storage in the UGen.
    if (!unit->m_outFramePrev) {
        // This is an annoying way to have to allocate memory,
        // but it seems to be necessary based on how SCPolarBuf is defined.
        float *outFramePrev = (float*)RTAlloc(unit->mWorld, stftBufFftSize * sizeof(float));
        float *frameNext = (float*)RTAlloc(unit->mWorld, stftBufFftSize * sizeof(float));
        float *framePrev1 = (float*)RTAlloc(unit->mWorld, stftBufFftSize * sizeof(float));
        float *framePrev2 = (float*)RTAlloc(unit->mWorld, stftBufFftSize * sizeof(float));
        unit->m_outFramePrev = (SCPolarBuf*)outFramePrev;
        unit->m_frameNext = (SCPolarBuf*)frameNext;
        unit->m_framePrev1 = (SCPolarBuf*)framePrev1;
        unit->m_framePrev2 = (SCPolarBuf*)framePrev2;
    }
    
    float startPos = sc_clip<float>(IN0(2), 0.0, 1.0);
    const float rate = IN0(3);
    const float loop = IN0(5);
    // std::cout << "Rate: " << rate << " Loop: " << loop << "\n";

    if (startPos != unit->m_startPos) {
        unit->m_startPos = startPos;
        unit->m_firstFrame = true;
        // std::cout << "Start pos changed\n";
    }

    // Now that we've run setup, we're ready to read STFT data and perform phase vocoder stretching.

    // First we need to figure out where we are, and if that means we need to loop or quit.
    const float newPos = unit->m_pos + rate;
    if (newPos > stftFrames - 1) {
        if (loop) {
            unit->m_firstFrame = true;
            startPos = 0;
        } else {
            OUT0(0) = -1.f;
            RELEASE_SNDBUF_SHARED(stftBuf);
            DoneAction(static_cast<int>(IN0(6)), unit);
            return;
        }
    }

    // The first frame has to be cloned directly from the STFT buffer with no phase adjustments.
    // This is essential to make sure that subsequent phase calculations are correctly aligned.
    if (unit->m_firstFrame) {
        // Compute the index of the first frame
        size_t firstFrameIdx = static_cast<size_t>(std::round(startPos * stftFrames));
        if (firstFrameIdx >= stftFrames) {
            if (loop) {
                firstFrameIdx = 0;
            } else {
                OUT0(0) = -1.f;
                RELEASE_SNDBUF_SHARED(stftBuf);
                DoneAction(static_cast<int>(IN0(6)), unit);
                return;
            }
        }

        // Copy the FFT data over
        SCPolarBuf *p = ToPolarApx(buf);
        const float *currentFftFrame = stftData + (firstFrameIdx * stftBufFftSize);
        // Fill the output buffer
        fillPolarBuf(currentFftFrame, p, stftBufFftSize);
        
        // We always need to store the resultant FFT frame in the UGen.
        // It is used in the next call to PV_PlayBufStretch_next, for 
        // phase vocoder calculations.
        fillPolarBuf(currentFftFrame, unit->m_outFramePrev, stftBufFftSize);

        // We have to advance by one frame because we need to be able to compute frequency for time stretching.
        unit->m_pos = static_cast<float>(firstFrameIdx + 1);
        unit->m_firstFrame = false;
    }
    
    // For frames other than the first frame, we'll need to perform phase computation.
    else {
        bool phaseLock = false;
        if (IN0(4) != 0.f) {
            phaseLock = true;
            // std::cout << "Phase lock\n";
        }
        SCPolarBuf *p = ToPolarApx(buf);
        size_t roundedPos = static_cast<size_t>(std::round(newPos));
        //std::cout << "New pos: " << intPos << "\n";
        if (std::abs(roundedPos-newPos) < 1e-3) {
            // If we're right smack on a specific FFT frame, we don't
            // need to do any magnitude or frequency interpolation, so
            // we only need the current and previous FFT frames from the buffer.
            size_t lastPos = roundedPos - 1;
            fillPolarBuf(stftData + (roundedPos * stftBufFftSize), unit->m_frameNext, stftBufFftSize);
            fillPolarBuf(stftData + (lastPos * stftBufFftSize), unit->m_framePrev1, stftBufFftSize);
            // Render the output FFT frame
            Stretch2(
                unit->m_frameNext, 
                unit->m_framePrev1, 
                p, 
                unit->m_outFramePrev, 
                stftBufFftSize, 
                stftBufHopSize, 
                phaseLock
            );
        } else {
            // Otherwise we're between two FFT frames, and we're going to have to
            // interpolate magnitude and frequency data.
            size_t lo = static_cast<size_t>(std::floor(newPos));
            size_t hi = static_cast<size_t>(std::ceil(newPos));
            // We are in between these two frames
            fillPolarBuf(stftData + (hi * stftBufFftSize), unit->m_frameNext, stftBufFftSize);
            fillPolarBuf(stftData + (lo * stftBufFftSize), unit->m_framePrev1, stftBufFftSize);
            // This is the frame right before that. It's needed to compute the previous instantaneous frequencies.
            fillPolarBuf(stftData + ((lo-1) * stftBufFftSize), unit->m_framePrev2, stftBufFftSize);
            // Render the output FFT frame
            Stretch3(
                unit->m_frameNext, 
                unit->m_framePrev1, 
                unit->m_framePrev2, 
                p, 
                unit->m_outFramePrev, 
                newPos-static_cast<float>(lo), 
                stftBufFftSize, 
                stftBufHopSize, 
                phaseLock
            );
        }
        // We always need to store the resultant FFT frame in the UGen.
        // It is used in the next call to PV_PlayBufStretch_next, for 
        // phase vocoder calculations.
        copyPolarBuf(p, unit->m_outFramePrev, static_cast<size_t>(numbins));
        unit->m_pos = newPos;
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
/// \param phaseLock Whether or not to apply phase locking
void Stretch2(
    const SCPolarBuf *frame, 
    const SCPolarBuf *framePrev, 
    SCPolarBuf *outFrame, 
    const SCPolarBuf *outFramePrev, 
    size_t fftSize, 
    size_t hopSize, 
    bool phaseLock) {
    outFrame->dc = frame->dc;
    outFrame->nyq = frame->nyq;
    for (size_t xxk = 0; xxk < fftSize/2-1; xxk++) {
        outFrame->bin[xxk].mag = frame->bin[xxk].mag;

        // Puckette-style phase locking
        if (phaseLock) {
            // Compute the instantaneous frequency
            float omegaK = twopi * (xxk+1) / fftSize;
            float phaseInc = frame->bin[xxk].phase - framePrev->bin[xxk].phase - hopSize * omegaK;
            phaseInc = std::fmod(phaseInc + pi, twopi) - pi;
            float instantaneousFreq = omegaK + phaseInc/hopSize;

            // In Puckette-style phase locking, we make a substitution for the previous phase,
            // in order to "lock" phases of adjacent bins together.
            float prevPhase = 0.0;
            if (xxk == 0) {
                std::complex<float> prevBinKMinus1 = std::polar<float>(outFramePrev->dc, 0.f);
                std::complex<float> prevBinK = std::polar<float>(outFramePrev->bin[xxk].mag, outFramePrev->bin[xxk].phase);
                std::complex<float> prevBinKPlus1 = std::polar<float>(outFramePrev->bin[xxk+1].mag, outFramePrev->bin[xxk+1].phase);
                prevPhase = std::arg(prevBinK - prevBinKMinus1 - prevBinKPlus1);
            } else if (xxk == fftSize/2-2) {
                std::complex<float> prevBinKMinus1 = std::polar<float>(outFramePrev->bin[xxk-1].mag, outFramePrev->bin[xxk-1].phase);
                std::complex<float> prevBinK = std::polar<float>(outFramePrev->bin[xxk].mag, outFramePrev->bin[xxk].phase);
                std::complex<float> prevBinKPlus1 = std::polar<float>(outFramePrev->nyq, 0.f);
                prevPhase = std::arg(prevBinK - prevBinKMinus1 - prevBinKPlus1);
            } else {
                std::complex<float> prevBinKMinus1 = std::polar<float>(outFramePrev->bin[xxk-1].mag, outFramePrev->bin[xxk-1].phase);
                std::complex<float> prevBinK = std::polar<float>(outFramePrev->bin[xxk].mag, outFramePrev->bin[xxk].phase);
                std::complex<float> prevBinKPlus1 = std::polar<float>(outFramePrev->bin[xxk+1].mag, outFramePrev->bin[xxk+1].phase);
                prevPhase = std::arg(prevBinK - prevBinKMinus1 - prevBinKPlus1);
            }
            
            // Compute the new phase
            outFrame->bin[xxk].phase = prevPhase + hopSize * instantaneousFreq;
        } else {
            // Compute the instantaneous frequency
            float omegaK = twopi * (xxk+1) / fftSize;
            float phaseInc = frame->bin[xxk].phase - framePrev->bin[xxk].phase - hopSize * omegaK;
            phaseInc = std::fmod(phaseInc + pi, twopi) - pi;
            float instantaneousFreq = omegaK + phaseInc/hopSize;
            
            // Compute the new phase
            outFrame->bin[xxk].phase = outFramePrev->bin[xxk].phase + hopSize * instantaneousFreq;
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
/// \param phaseLock Whether or not to apply phase locking
void Stretch3(
    const SCPolarBuf *frameNext, 
    const SCPolarBuf *framePrev1,
    const SCPolarBuf *framePrev2, 
    SCPolarBuf *outFrame,
    const SCPolarBuf *outFramePrev,
    float pos,
    size_t fftSize, 
    size_t hopSize, 
    bool phaseLock) {
    outFrame->dc = INTERP(framePrev1->dc, frameNext->dc, pos);
    outFrame->nyq = INTERP(framePrev1->nyq, frameNext->nyq, pos);
    for (size_t xxk = 0; xxk < fftSize/2-1; xxk++) {
        outFrame->bin[xxk].mag = INTERP(framePrev1->bin[xxk].mag, frameNext->bin[xxk].mag, pos);

        // Puckette-style phase locking
        if (phaseLock) {
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
                std::complex<float> prevBinKMinus1 = std::polar<float>(outFramePrev->dc, 0.f);
                std::complex<float> prevBinK = std::polar<float>(outFramePrev->bin[xxk].mag, outFramePrev->bin[xxk].phase);
                std::complex<float> prevBinKPlus1 = std::polar<float>(outFramePrev->bin[xxk+1].mag, outFramePrev->bin[xxk+1].phase);
                prevPhase = std::arg(prevBinK - prevBinKMinus1 - prevBinKPlus1);
            } else if (xxk == fftSize/2-2) {
                std::complex<float> prevBinKMinus1 = std::polar<float>(outFramePrev->bin[xxk-1].mag, outFramePrev->bin[xxk-1].phase);
                std::complex<float> prevBinK = std::polar<float>(outFramePrev->bin[xxk].mag, outFramePrev->bin[xxk].phase);
                std::complex<float> prevBinKPlus1 = std::polar<float>(outFramePrev->nyq, 0.f);
                prevPhase = std::arg(prevBinK - prevBinKMinus1 - prevBinKPlus1);
            } else {
                std::complex<float> prevBinKMinus1 = std::polar<float>(outFramePrev->bin[xxk-1].mag, outFramePrev->bin[xxk-1].phase);
                std::complex<float> prevBinK = std::polar<float>(outFramePrev->bin[xxk].mag, outFramePrev->bin[xxk].phase);
                std::complex<float> prevBinKPlus1 = std::polar<float>(outFramePrev->bin[xxk+1].mag, outFramePrev->bin[xxk+1].phase);
                prevPhase = std::arg(prevBinK - prevBinKMinus1 - prevBinKPlus1);
            }
            
            // Compute the new phase
            outFrame->bin[xxk].phase = prevPhase + hopSize * instantaneousFreq;
        } else {
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
            outFrame->bin[xxk].phase = outFramePrev->bin[xxk].phase + hopSize * instantaneousFreq;
        }
    }
}

/// Fills a SCPolarBuf with saved STFT data from a single frame
///
/// \param fftBuf The FFT frame from the STFT buffer
/// \param [out] polarBuf The SCPolarBuf to copy to
/// \param fftSize The FFT size
void fillPolarBuf(const float *fftBuf, SCPolarBuf *polarBuf, size_t fftSize) {
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
void copyPolarBuf(const SCPolarBuf *sourceBuf, SCPolarBuf *destBuf, size_t numbins) {
    destBuf->dc = sourceBuf->dc;
    destBuf->nyq = sourceBuf->nyq;
    for (size_t xxn = 0; xxn < numbins; xxn++) {
        destBuf->bin[xxn].mag = sourceBuf->bin[xxn].mag;
        destBuf->bin[xxn].phase = sourceBuf->bin[xxn].phase;
    }
}