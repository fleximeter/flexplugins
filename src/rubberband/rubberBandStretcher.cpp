/*
File: rubberBandStretcher.cpp
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

#include "rubberBandStretcher.hpp"
#include <limits>
#include "SC_PlugIn.h"

extern InterfaceTable *ft;

void RubberBandStretcher_Ctor(RubberBandStretcher *unit) {
    float timeRatio = IN0(1);
    float pitchRatio = IN0(2);
    float formantRatio = IN0(3);
    int transientsMode = static_cast<int>(IN0(4));
    int detector = static_cast<int>(IN0(5));
    int phaseOption = static_cast<int>(IN0(6));
    int pitchQuality = static_cast<int>(IN0(7));
    int windowOption = static_cast<int>(IN0(8));
    int smoothing = static_cast<int>(IN0(9));
    int engine = static_cast<int>(IN0(10));
    
    unit->m_timeRatio = timeRatio;
    unit->m_pitchRatio = pitchRatio;
    unit->m_formantRatio = formantRatio;
    unit->m_transientsMode = transientsMode;
    unit->m_detectorOption = detector;
    unit->m_phaseOption = phaseOption;
    unit->m_pitchQuality = pitchQuality;

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
    unit->m_stretcher->setTimeRatio(sc_clip(timeRatio, 1.f, std::numeric_limits<float>::infinity()));
    unit->m_stretcher->setPitchScale(sc_clip(pitchRatio, 1e-2, 64));
    unit->m_stretcher->setFormantScale(sc_clip(formantRatio, 1e-2, 64));

    // Feed samples in until the shifter is ready to start producing valid output.
    // This is necessary because the shifter isn't ready to produce valid output
    // as soon as it is initialized--it requires padded 0s to be fed in for some
    // number of samples specified by the shifter.
    float *zeroBuf = (float*)RTAlloc(unit->mWorld, BUFLENGTH * sizeof(float));
    for (size_t i = 0; i < BUFLENGTH; i++) {
        zeroBuf[i] = 0.f;
    }

    // The number of initial zeros required
    size_t startPad = unit->m_stretcher->getPreferredStartPad();
    
    // The number of samples to discard at the beginning of the stretcher output.
    // This is handled in the RubberBandStretcher_next() method.
    unit->m_samplesToDiscard = unit->m_stretcher->getStartDelay();

    // Feed in the start pad samples
    while (startPad > 0) {
        unit->m_stretcher->process(&zeroBuf, BUFLENGTH, false);
        startPad -= BUFLENGTH;
    }
    RTFree(unit->mWorld, zeroBuf);

    // Initialize first out sample
    OUT0(0) = 0;

    SETCALC(RubberBandStretcher_next);
}

void RubberBandStretcher_Dtor(RubberBandStretcher *unit) {
    RTFree(unit->mWorld, unit->m_stretcher);
}

void RubberBandStretcher_next(RubberBandStretcher *unit, int inNumSamples) {
    float *in = IN(0);
    float *out = OUT(0);
    float timeRatio = IN0(1);
    float pitchRatio = IN0(2);
    float formantRatio = IN0(3);
    int transientsMode = static_cast<int>(IN0(4));
    int detector = static_cast<int>(IN0(5));
    int phaseOption = static_cast<int>(IN0(6));
    int pitchQuality = static_cast<int>(IN0(7));

    // Update shifter options only if something has changed
    if (timeRatio != unit->m_timeRatio) {
        unit->m_stretcher->setTimeRatio(sc_clip(timeRatio, 1.f, std::numeric_limits<float>::infinity()));
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

    unit->m_stretcher->process(&in, BUFLENGTH, false);
    
    // If we can retrieve a full block worth of audio
    if (unit->m_stretcher->available() >= BUFLENGTH) {
        unit->m_stretcher->retrieve(&out, BUFLENGTH);
        // Clear initial samples if necessary
        if (unit->m_samplesToDiscard > 0) {
            size_t i = 0;
            while (i < BUFLENGTH && unit->m_samplesToDiscard > 0) {
                out[i] = 0.f;
                unit->m_samplesToDiscard--;
                i++;
            }
        }
    }
    
    // Output zeros if the shifter has no new samples available
    else {
        for (size_t i = 0; i < BUFLENGTH; i++) {
            out[i] = 0.f;
        }
    }
}
