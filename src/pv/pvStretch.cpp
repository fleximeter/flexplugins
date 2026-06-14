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
#include "SC_InterfaceTable.h"
#include "FFT_UGens.h"
#include <iostream>

extern InterfaceTable *ft;

void PV_PlayBufStretch_Ctor(PV_PlayBufStretch *unit) {
    // Connect to the STFT buffer. For now, we only allow this in the constructor.
    float fbufnum = IN0(1);
    uint32 bufnum = static_cast<uint32>(fbufnum);
    if (bufnum >= unit->mWorld->mNumSndBufs) bufnum = 0;
    unit->m_fbufnum = fbufnum;
    unit->m_buf = unit->mWorld->mSndBufs + bufnum;
    
    // Configure position
    float startPos = sc_clip<float>(IN0(2), 0.0, 1.0);
    unit->m_pos = 0.f;
    unit->m_startPos = startPos;
    unit->m_firstFrame = true;

    SETCALC(PV_PlayBufStretch_next);
    OUT0(0) = IN0(0);
}

void PV_PlayBufStretch_next(PV_PlayBufStretch *unit, int inNumSamples) {
    PV_GET_BUF
    float startPos = sc_clip<float>(IN0(2), 0.0, 1.0);
    float rate = IN0(3);
    float loop = IN0(4);

    // This section of the code is for acquiring the STFT buffer and information about it.
    // It has to be run every time because we cannot be sure the user has not freed the buffer.
    // We also have to verify important details about the buffer to make sure we can read from it at all.
    const SndBuf *stftBuf = unit->m_buf;
    if (!stftBuf) {
        OUT0(0) = -1.f;
        std::cout << "WARNING: The stftBuf could not be accessed. Aborting.\n";
        return;
    }
    ACQUIRE_SNDBUF_SHARED(stftBuf);
    const float* bufData __attribute__((__unused__)) = stftBuf->data;
    const float* stftData __attribute__((__unused__)) = stftBuf->data + 3;
    uint32 bufChannels __attribute__((__unused__)) = stftBuf->channels;
    uint32 bufSamples __attribute__((__unused__)) = stftBuf->samples;
    uint32 bufFrames = stftBuf->frames;
    // first 3 frames have analysis parameters
    int stftFrames = (static_cast<int>(stftBuf->samples) - 3) / static_cast<int>(buf->samples);

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
    size_t stftBufFftSize = bufData[0];
    size_t stftBufHopSize = static_cast<float>(bufData[1] * stftBufFftSize);  // in frames, not fraction
    int stftBufWinType = static_cast<int>(bufData[2]);  // -1, 0, or 1. This information is probably extraneous.

    if (stftBufFftSize != buf->samples) {
        OUT0(0) = -1.f;
        std::cout << "WARNING: The FFT size of the STFT buffer (" << stftBufFftSize << 
            ") does not match the FFT size of the PV_PlayBufStretch (" << 
            buf->samples << "). Aborting.\n";
        RELEASE_SNDBUF_SHARED(stftBuf);
        return;
    }

    if (startPos != unit->m_startPos) {
        unit->m_startPos = startPos;
        unit->m_firstFrame = true;
    }

    // Now that we've run setup, we're ready to read STFT data and perform phase vocoder stretching.

    // The first frame has to be cloned directly from the STFT buffer with no phase adjustments.
    // This is essential to make sure that subsequent phase calculations are correctly aligned.
    if (unit->m_firstFrame) {
        // Compute the index of the first frame
        size_t xxi = static_cast<size_t>(std::round(startPos * stftFrames));
        if (xxi >= stftFrames) {
            if (loop) {
                xxi = 0;
            } else {
                OUT0(0) = -1.f;
                RELEASE_SNDBUF_SHARED(stftBuf);
                DoneAction(static_cast<int>(IN0(5)), unit);
                return;
            }
        }

        // Copy the FFT data over
        SCPolarBuf *p = ToPolarApx(buf);
        const float *currentFftFrame = stftData + (xxi * stftBufFftSize);
        p->dc = currentFftFrame[0];
        p->nyq = currentFftFrame[1];
        for (size_t xxn = 2, xxk = 0; xxn < stftBufFftSize; xxn+=2, xxk++) {
            // For some reason the phase is stored first, then the magnitude.
            // This prevents a direct cast to SCPolarBuf, unfortunately.
            p->bin[xxk].phase = currentFftFrame[xxn];
            p->bin[xxk].mag = currentFftFrame[xxn+1];
        }

        // We have to advance by one frame because we need to be able to compute frequency for time stretching.
        unit->m_pos = static_cast<float>(xxi + 1);
    }
    
    // For frames other than the first frame, we'll need to perform phase computation.
    else {
        float newPos;
    }

    RELEASE_SNDBUF_SHARED(stftBuf);

    // if no loop and we're past the last frame, handle this condition later
    // DoneAction(static_cast<int>(IN0(5)), unit);
}