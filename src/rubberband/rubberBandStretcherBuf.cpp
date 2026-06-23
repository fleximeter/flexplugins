/*
File: rubberBandStretcherBuf.cpp
Author: Jeff Martin

Description:
A high-quality formant preserving pitch shifter and time stretcher using the RubberBand library.

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

#include "rubberBandStretcherBuf.hpp"
#include <iostream>
#include "SC_PlugIn.h"

extern InterfaceTable *ft;

void RubberBandStretcherBuf_Ctor(RubberBandStretcherBuf *unit) {

    /* arg in, bufnum=0, offset=0.0, recLevel=1.0, preLevel=0.0, run=1.0, loop=1.0, trigger=1.0, 
        8    timeRatio=1.0, pitchRatio=1.0, formantRatio=0.0, transientsMode=0, 
        12    detectorMode=0, phaseMode=0, pitchQuality=0, windowOption=0, 
        16    smoothing=0, engine=0, doneAction=0;
    */

    float timeRatio = IN0(8);
    float pitchRatio = IN0(9);
    float formantRatio = IN0(10);
    int transientsMode = static_cast<int>(IN0(11));
    int detector = static_cast<int>(IN0(12));
    int phaseOption = static_cast<int>(IN0(13));
    int pitchQuality = static_cast<int>(IN0(14));
    int windowOption = static_cast<int>(IN0(15));
    int smoothing = static_cast<int>(IN0(16));
    int engine = static_cast<int>(IN0(17));
    
    unit->m_timeRatio = timeRatio;
    unit->m_pitchRatio = pitchRatio;
    unit->m_formantRatio = formantRatio;
    unit->m_transientsMode = transientsMode;
    unit->m_detectorOption = detector;
    unit->m_phaseOption = phaseOption;
    unit->m_pitchQuality = pitchQuality;

    // Acquire the sound buffer
    float fbufnum = IN0(1);
    uint32 bufnum = static_cast<uint32>(fbufnum);
    if (bufnum >= unit->mWorld->mNumSndBufs) bufnum = 0;
    unit->m_fbufnum = fbufnum;
    unit->m_buf = unit->mWorld->mSndBufs + bufnum;
    unit->m_writePtr = static_cast<size_t>(IN0(2));  // initial offset
    unit->m_prevTrigger = 0.f;

    // Set up RubberBandStretcher initial options
    int options = 0x01000001;  // formant-preserving, real-time options set
    switch (transientsMode) {
        case 1:
            options |= 0x00000100;
            break;
        case 2:
            options |= 0x00000200;
            break;
    }
    switch (detector) {
        case 1:
            options |= 0x00000400;
            break;
        case 2:
            options |= 0x00000800;
            break;
    }
    switch (phaseOption) {
        case 1:
            options |= 0x00002000;
    }
    switch (pitchQuality) {
        case 1:
            options |= 0x02000000;
            break;
        case 2:
            options |= 0x04000000;
            break;
    }
    switch (windowOption) {
        case 1:
            options |= 0x00100000;
            break;
        case 2:
            options |= 0x00200000;
            break;
    }
    switch (smoothing) {
        case 1:
            options |= 0x00800000;
            break;
    }
    switch (engine) {
        case 1:
            options |= 0x20000000;
            break;
    }

    // Allocate the shifter with the given options
    unit->m_stretcher = (RubberBand::RubberBandStretcher*)RTAlloc(unit->mWorld, sizeof(RubberBand::RubberBandStretcher));
    new (unit->m_stretcher) RubberBand::RubberBandStretcher(static_cast<size_t>(SAMPLERATE), 1, options, timeRatio, pitchRatio);

    // Initialize the shifter
    // The shifter accepts a block size (which must be set before the first process()
    // call and not after), which avoids the need to use local RingBuffers.
    unit->m_stretcher->setMaxProcessSize(BUFLENGTH);
    unit->m_stretcher->setTimeRatio(sc_clip(timeRatio, 1e-5, 1e5));
    unit->m_stretcher->setPitchScale(sc_clip(pitchRatio, 1e-2, 64));
    unit->m_stretcher->setFormantScale(sc_clip(formantRatio, 1e-2, 64));

    // Feed samples in until the shifter is ready to start producing valid output.
    // This is necessary because the shifter isn't ready to produce valid output
    // as soon as it is initialized--it requires padded 0s to be fed in for some
    // number of samples specified by the shifter.
    unit->m_localBuf = (float*)RTAlloc(unit->mWorld, BUFLENGTH * sizeof(float));
    for (size_t i = 0; i < BUFLENGTH; i++) {
        unit->m_localBuf[i] = 0.f;
    }

    // The number of initial zeros required
    size_t startPad = unit->m_stretcher->getPreferredStartPad();
    
    // The number of samples to discard at the beginning of the stretcher output.
    // This is handled in the RubberBandStretcher_next() method.
    unit->m_samplesToDiscard = unit->m_stretcher->getStartDelay();

    // Feed in the start pad samples
    while (startPad > 0) {
        unit->m_stretcher->process(&unit->m_localBuf, BUFLENGTH, false);
        startPad -= BUFLENGTH;
    }

    // Initialize first out sample
    OUT0(0) = 0;

    SETCALC(RubberBandStretcherBuf_next);
}

void RubberBandStretcherBuf_Dtor(RubberBandStretcherBuf *unit) {
    if (unit->m_stretcher) RTFree(unit->mWorld, unit->m_stretcher);
    if (unit->m_localBuf) RTFree(unit->mWorld, unit->m_localBuf);
}

void RubberBandStretcherBuf_next(RubberBandStretcherBuf *unit, int inNumSamples) {
    /* arg in, bufnum=0, offset=0.0, recLevel=1.0, preLevel=0.0, run=1.0, loop=1.0, trigger=1.0, 
        8    timeRatio=1.0, pitchRatio=1.0, formantRatio=0.0, transientsMode=0, 
        12    detectorMode=0, phaseMode=0, pitchQuality=0, windowOption=0, 
        16    smoothing=0, engine=0, doneAction=0;
    */

    // Step 1: acquire the sound buffer
    const SndBuf *writeBuf = unit->m_buf;
    if (!writeBuf) {
        std::cout << "WARNING: The stftBuffer could not be accessed. Aborting.\n";
        ClearUnitOutputs(unit, inNumSamples);
        return;
    }
    ACQUIRE_SNDBUF_SHARED(writeBuf);
    float* bufData __attribute__((__unused__)) = writeBuf->data;
    const uint32 bufChannels __attribute__((__unused__)) = writeBuf->channels;
    const uint32 bufSamples __attribute__((__unused__)) = writeBuf->samples;
    const uint32 bufFrames = writeBuf->frames;
    
    if (bufChannels != 1) {
        std::cout << "WARNING: The buffer has " << bufChannels << " channels, but the " << 
            "RubberBandStretcherBuf only supports mono buffers. Aborting.\n";
        ClearUnitOutputs(unit, inNumSamples);
        RELEASE_SNDBUF_SHARED(writeBuf);
        return;
    }

    // 2. Update unit parameters if required
    float timeRatio = IN0(8);
    float pitchRatio = IN0(9);
    float formantRatio = IN0(10);
    int transientsMode = static_cast<int>(IN0(11));
    int detector = static_cast<int>(IN0(12));
    int phaseOption = static_cast<int>(IN0(13));
    int pitchQuality = static_cast<int>(IN0(14));

    // Update shifter options only if something has changed
    if (timeRatio != unit->m_timeRatio) {
        unit->m_stretcher->setTimeRatio(sc_clip(timeRatio, 1e-5, 1e5));
    }
    if (pitchRatio != unit->m_pitchRatio) {
        unit->m_stretcher->setPitchScale(sc_clip(pitchRatio, 1e-2, 64));
    }
    if (formantRatio != unit->m_formantRatio) {
        unit->m_stretcher->setFormantScale(sc_clip(formantRatio, 1e-2, 64));
    }
    // QUESTION: Will this method of setting options override all existing options,
    // or just the option provided? May need to compute all options from scratch.
    if (transientsMode != unit->m_transientsMode) {
        switch (transientsMode) {
            case 1:
                unit->m_stretcher->setTransientsOption(0x00000100);
                break;
            case 2:
                unit->m_stretcher->setTransientsOption(0x00000200);
                break;
            default:
                unit->m_stretcher->setTransientsOption(0x00000000);
        }
    }
    if (detector != unit->m_detectorOption) {
        switch (detector) {
            case 1:
                unit->m_stretcher->setDetectorOption(0x00000400);
                break;
            case 2:
                unit->m_stretcher->setDetectorOption(0x00000800);
                break;
            default:
                unit->m_stretcher->setDetectorOption(0x00000000);
        }
    }
    if (phaseOption != unit->m_phaseOption) {
        switch (phaseOption) {
            case 1:
                unit->m_stretcher->setPhaseOption(0x00002000);
                break;
            default:
                unit->m_stretcher->setPhaseOption(0x00000000);
        }
    }
    if (pitchQuality != unit->m_pitchQuality) {
        switch (pitchQuality) {
            case 1:
                unit->m_stretcher->setPitchOption(0x02000000);
                break;
            case 2:
                unit->m_stretcher->setPitchOption(0x04000000);
                break;
            default:
                unit->m_stretcher->setPitchOption(0x00000000);
        }
    }

    // 3. Handle the trigger functionality
    float trigger = IN0(7);
    if (trigger > 0.f && unit->m_prevTrigger <= 0.f) {
        unit->m_writePtr = 0;
    }
    unit->m_prevTrigger = trigger;

    // 4. Process input audio.
    float *in = IN(0);
    float recLevel = IN0(3);
    float preLevel = IN0(4);
    float loop = IN0(6);
    
    unit->m_stretcher->process(&in, BUFLENGTH, false);

    while (unit->m_stretcher->available() > 0) {
        size_t numRetrieve = static_cast<size_t>(std::min(BUFLENGTH, unit->m_stretcher->available()));
        unit->m_stretcher->retrieve(&unit->m_localBuf, numRetrieve);
        // 5. While we run the audio through the stretcher regardless, we only store it to the buffer if "run" is set.
        if (IN0(5) > 0.f) {
            for (size_t xxi = 0; xxi < numRetrieve; xxi++) {
                if (unit->m_writePtr >= bufSamples) {
                    if (loop > 0.f) {
                        unit->m_writePtr = 0;
                    } else {
                        RELEASE_SNDBUF_SHARED(writeBuf);
                        ClearUnitOutputs(unit, inNumSamples);
                        DoneAction(static_cast<int>(IN0(18)), unit);
                        return;
                    }
                }

                bufData[unit->m_writePtr] = preLevel * bufData[unit->m_writePtr] + recLevel * unit->m_localBuf[xxi];
                unit->m_writePtr++;
            }
        }
    }

    ClearUnitOutputs(unit, inNumSamples);
    RELEASE_SNDBUF_SHARED(writeBuf);
}