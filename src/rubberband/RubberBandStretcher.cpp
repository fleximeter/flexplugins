/*
File: rubberband.cpp
Author: Jeff Martin

Description:
A high quality, formant-preserving live pitch shifter using the RubberBand library.

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

#include "SC_InterfaceTable.h"
#include "SC_Unit.h"
#include "rubberband/RubberBandStretcher.h"
#include <iostream>

InterfaceTable *ft;

struct RubberBandStretcher : public Unit {
    RubberBand::RubberBandStretcher* m_stretcher;
};

static void RubberBandStretcher_next(RubberBandStretcher *unit, int inNumSamples) {
    float *in = IN(0);
    float *out = OUT(0);
    float timeRatio = IN0(1);
    float pitchRatio = IN0(2);
    float formantRatio = IN0(3);
    int transientsMode = IN0(4);
    int detector = IN0(5);
    int phaseOption = IN0(6);
    int pitchQuality = IN0(7);
}

static void RubberBandStretcher_Ctor(RubberBandStretcher *unit) {
    float timeRatio = IN0(1);
    float pitchRatio = IN0(2);
    float formantRatio = IN0(3);
    int transientsMode = IN0(4);
    int detector = IN0(5);
    int phaseOption = IN0(6);
    int pitchQuality = IN0(7);
    int windowOption = IN0(8);
    int smoothing = IN0(9);
    int engine = IN0(10);

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
    new (unit->m_stretcher) RubberBand::RubberBandStretcher(SAMPLERATE, 1, options, timeRatio, pitchRatio);

    // Initialize the shifter
    std::cout << "Preferred start pad: " << unit->m_stretcher->getPreferredStartPad() << std::endl;
    std::cout << "Start delay: " << unit->m_stretcher->getStartDelay() << std::endl;
    //std::cout << "Initial samples required: " << unit->m_stretcher->getSamplesRequired() << std::endl;

    // Initialize first out sample
    OUT0(0) = 0;

    SETCALC(RubberBandStretcher_next);
}

static void RubberBandStretcher_Dtor(RubberBandStretcher *unit) {
    RTFree(unit->mWorld, unit->m_stretcher);
}

PluginLoad(PV_Jeff) {
    ft = inTable;
    DefineDtorUnit(RubberBandStretcher);
}
