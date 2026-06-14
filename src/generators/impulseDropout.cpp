/*
File: impulseDropout.cpp
Author: Jeff Martin

Description:
The ImpulseDropout UGen

Copyright © 2025 by Jeffrey Martin. All rights reserved.
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

#include "impulseDropout.hpp"
extern InterfaceTable *ft;

// This is a copy of the static function from LFUGens.cpp in server/plugins.
// It detects if a phasor is out-of-bounds, triggers, and wraps [0, 1].
static inline float testWrapPhase(double prev_inc, double& phase) {
    if (prev_inc < 0.f) { // negative freqs
        if (phase <= 0.f) {
            phase += 1.f;
            if (phase <= 0.f) { // catch large phase jumps
                phase -= sc_ceil(phase);
            }
            return 1.f;
        } else {
            return 0.f;
        }
    } else { // positive freqs
        if (phase >= 1.f) {
            phase -= 1.f;
            if (phase >= 1.f) {
                phase -= sc_floor(phase);
            }
            return 1.f;
        } else {
            return 0.f;
        }
    }
}

void ImpulseDropout_next_aa(ImpulseDropout* unit, int inNumSamples) {
    float* out = OUT(0);
    float* freqIn = IN(0);
    float* offIn = IN(1);
    float dropProbIn = IN0(2);
    
    // Collect UGen state
    double phase = unit->mPhase;
    double inc = unit->mPhaseIncrement;
    float freqMul = unit->mFreqMul;
    double prevOff = unit->mPhaseOffset;
    
    RGET
    for (int xxn = 0; xxn < inNumSamples; xxn++) {
        float impulseResult = testWrapPhase(inc, phase);
        // Drop the impulse if necessary
        if (impulseResult > 0.5f && rgen.frand() < dropProbIn) {
            impulseResult = 0.f;
        }
        double off = static_cast<double>(offIn[xxn]);
        double offInc = off - prevOff;
        phase += offInc;
        testWrapPhase(inc, phase);
        inc = freqIn[xxn] * freqMul;
        out[xxn] = impulseResult;
        phase += inc;
        prevOff = off;
    }

    unit->mPhase = phase;
    unit->mPhaseOffset = prevOff;
    unit->mPhaseIncrement = inc;
}

void ImpulseDropout_next_ai(ImpulseDropout* unit, int inNumSamples) {
    float* out = OUT(0);
    float freqIn = IN0(0);
    float dropProbIn = IN0(2);

    // Collect UGen state
    double phase = unit->mPhase;
    double inc = unit->mPhaseIncrement;
    float freqMul = unit->mFreqMul;

    RGET
    for (int xxn = 0; xxn < inNumSamples; xxn++) {
        float impulseResult = testWrapPhase(inc, phase);
        // Drop the impulse if necessary
        if (impulseResult > 0.5f && rgen.frand() < dropProbIn) {
            impulseResult = 0.f;
        }
        inc = freqIn * freqMul;
        out[xxn] = impulseResult;
        phase += inc;
    }

    unit->mPhase = phase;
    unit->mPhaseIncrement = inc;
}

void ImpulseDropout_next_ak(ImpulseDropout* unit, int inNumSamples) {
    float* out = OUT(0);
    float freqIn = IN0(0);
    double off = IN0(1);
    float dropProbIn = IN0(2);
    
    // Collect UGen state
    double phase = unit->mPhase;
    double inc = unit->mPhaseIncrement;
    float freqMul = unit->mFreqMul;
    double prevOff = unit->mPhaseOffset;

    double offSlope = CALCSLOPE(off, prevOff);
    bool offChanged = offSlope != 0.f;

    RGET
    for (int xxn = 0; xxn < inNumSamples; xxn++) {
        float impulseResult = testWrapPhase(inc, phase);
        // Drop the impulse if necessary
        if (impulseResult > 0.5f && rgen.frand() < dropProbIn) {
            impulseResult = 0.f;
        }
        if (offChanged) {
            phase += offSlope;
            testWrapPhase(inc, phase);
        }
        inc = freqIn * freqMul;
        out[xxn] = impulseResult;
        phase += inc;
    }

    unit->mPhase = phase;
    unit->mPhaseOffset = off;
    unit->mPhaseIncrement = inc;
}

void ImpulseDropout_next_ki(ImpulseDropout* unit, int inNumSamples) {
    float* out = OUT(0);
    double inc = IN0(0) * unit->mFreqMul;
    float dropProbIn = IN0(2);

    // Collect UGen state
    double phase = unit->mPhase;
    double prevInc = unit->mPhaseIncrement;
    
    double incSlope = CALCSLOPE(inc, prevInc);
    
    RGET
    for (int xxn = 0; xxn < inNumSamples; xxn++) {
        float impulseResult = testWrapPhase(prevInc, phase);
        // Drop the impulse if necessary
        if (impulseResult > 0.5f && rgen.frand() < dropProbIn) {
            impulseResult = 0.f;
        }
        out[xxn] = impulseResult;
        prevInc += incSlope;
        phase += prevInc;
    }

    unit->mPhase = phase;
    unit->mPhaseIncrement = inc;
}

void ImpulseDropout_next_kk(ImpulseDropout* unit, int inNumSamples) {
    float* out = OUT(0);
    double inc = IN0(0) * unit->mFreqMul;
    double off = IN0(1);
    float dropProbIn = IN0(2);

    // Collect UGen state
    double phase = unit->mPhase;
    double prevInc = unit->mPhaseIncrement;
    double prevOff = unit->mPhaseOffset;
    
    double incSlope = CALCSLOPE(inc, prevInc);
    double phaseSlope = CALCSLOPE(off, prevOff);
    bool phOffChanged = phaseSlope != 0.f;

    RGET
    for (int xxn = 0; xxn < inNumSamples; xxn++) {
        float impulseResult = testWrapPhase(prevInc, phase);
        // Drop the impulse if necessary
        if (impulseResult > 0.5f && rgen.frand() < dropProbIn) {
            impulseResult = 0.f;
        }
        if (phOffChanged) {
            phase += phaseSlope;
            testWrapPhase(prevInc, phase);
        }
        out[xxn] = impulseResult;
        prevInc += incSlope;
        phase += prevInc;
    }

    unit->mPhase = phase;
    unit->mPhaseOffset = off;
    unit->mPhaseIncrement = inc;
}

void ImpulseDropout_next_ii(ImpulseDropout* unit, int inNumSamples) {
    float* out = OUT(0);
    float dropProbIn = IN0(2);
    
    // Collect UGen state
    double inc = unit->mPhaseIncrement;
    double phase = unit->mPhase;
        
    RGET
    for (int xxn = 0; xxn < inNumSamples; xxn++) {
        float impulseResult = testWrapPhase(inc, phase);
        // Drop the impulse if necessary
        if (impulseResult > 0.5f && rgen.frand() < dropProbIn) {
            impulseResult = 0.f;
        }
        out[xxn] = impulseResult;
        phase += inc;
    }

    unit->mPhase = phase;
}

void ImpulseDropout_next_ik(ImpulseDropout* unit, int inNumSamples) {
    float* out = OUT(0);
    double off = IN0(1);
    float dropProbIn = IN0(2);

    // Collect UGen state
    double phase = unit->mPhase;
    double inc = unit->mPhaseIncrement;
    double prevOff = unit->mPhaseOffset;

    double phaseSlope = CALCSLOPE(off, prevOff);
    bool phOffChanged = phaseSlope != 0.f;

    RGET
    for (int xxn = 0; xxn < inNumSamples; xxn++) {
        float impulseResult = testWrapPhase(inc, phase);
        // Drop the impulse if necessary
        if (impulseResult > 0.5f && rgen.frand() < dropProbIn) {
            impulseResult = 0.f;
        }
        if (phOffChanged) {
            phase += phaseSlope;
            testWrapPhase(inc, phase);
        }
        out[xxn] = impulseResult;
        phase += inc;
    }

    unit->mPhase = phase;
    unit->mPhaseOffset = off;
}

// Construct the ImpulseDropout
void ImpulseDropout_Ctor(ImpulseDropout* unit) {
    unit->mPhaseIncrement = IN0(0) * unit->mFreqMul;
    unit->mPhaseOffset = IN0(1);
    unit->mFreqMul = static_cast<float>(unit->mRate->mSampleDur);

    double initOff = unit->mPhaseOffset;
    double initInc = unit->mPhaseIncrement;
    double initPhase = sc_wrap(initOff, 0.0, 1.0);

    // Initial phase offset of 0 means output of 1 on first sample.
    // Set phase to wrap point to trigger impulse on first sample
    if (initPhase == 0.0 && initInc >= 0.0) {
        initPhase = 1.0; // positive frequency trigger/wrap position
    }
    unit->mPhase = initPhase;

    UnitCalcFunc func;
    switch (INRATE(0)) {
    case calc_FullRate:
        switch (INRATE(1)) {
        case calc_ScalarRate:
            func = (UnitCalcFunc)ImpulseDropout_next_ai;
            break;
        case calc_BufRate:
            func = (UnitCalcFunc)ImpulseDropout_next_ak;
            break;
        case calc_FullRate:
            func = (UnitCalcFunc)ImpulseDropout_next_aa;
            break;
        }
        break;
    case calc_BufRate:
        if (INRATE(1) == calc_ScalarRate) {
            func = (UnitCalcFunc)ImpulseDropout_next_ki;
        } else {
            func = (UnitCalcFunc)ImpulseDropout_next_kk;
        }
        break;
    case calc_ScalarRate:
        if (INRATE(1) == calc_ScalarRate) {
            func = (UnitCalcFunc)ImpulseDropout_next_ki;
        } else {
            func = (UnitCalcFunc)ImpulseDropout_next_kk;
        }
        break;
    }
    unit->mCalcFunc = func;
    func(unit, 1);

    unit->mPhase = initPhase;
    unit->mPhaseOffset = initOff;
    unit->mPhaseIncrement = initInc;
}