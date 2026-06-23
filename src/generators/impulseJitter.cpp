/*
File: impulseJitter.cpp
Author: Jeff Martin

Description:
The ImpulseJitter UGen

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

#include "impulseJitter.hpp"
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


void ImpulseJitter_next_aa(ImpulseJitter* unit, int inNumSamples) {
    float* out = OUT(0);
    float* freq = IN(0);
    float* offIn = IN(1);
    float jitterFracIn = IN0(2);
    
    // Collect UGen state
    double phase = unit->mPhase;
    double inc = unit->mPhaseIncrement;
    float freqMul = unit->mFreqMul;
    double prevOff = unit->mPhaseOffset;
    
    // The maximum distance an impulse can be displaced
    int jitterWidth = static_cast<int>(jitterFracIn * unit->mImpulseHeap.maxSize);

    // Zero out the output buffer
    for (int xxn = 0; xxn < inNumSamples; xxn++) {
        out[xxn] = 0.f;
    }

    // Update the impulse table indices
    for (int xxn = 1; xxn < unit->mImpulseHeap.size; xxn++) {
        unit->mImpulseHeap.heap[xxn] -= inNumSamples;
    }

    // Retrieve impulses for this block
    while (heapPeek(&unit->mImpulseHeap) < inNumSamples && unit->mImpulseHeap.size > 1) {
        out[heapPop(&unit->mImpulseHeap)] = 1.f;
    }

    RGET
    for (int xxn = 0; xxn < inNumSamples; xxn++) {
        size_t heapEffectiveSize = static_cast<size_t>(HEAP_MAX_SIZE / (12 * sc_log2(freq[xxn])));
        float impulseResult = testWrapPhase(inc, phase);
        if (impulseResult > 0.5f) {
            int idx = rgen.irand(jitterWidth) + xxn;
            if (idx < inNumSamples) {
                out[idx] = 1.f;
            } else if (unit->mImpulseHeap.size < heapEffectiveSize) {
                heapInsert(&unit->mImpulseHeap, idx);
            }
        }
        double off = static_cast<double>(offIn[xxn]);
        double offInc = off - prevOff;
        phase += offInc;
        testWrapPhase(inc, phase);
        inc = freq[xxn] * freqMul;
        phase += inc;
        prevOff = off;
    }

    unit->mPhase = phase;
    unit->mPhaseOffset = prevOff;
    unit->mPhaseIncrement = inc;
}

void ImpulseJitter_next_ai(ImpulseJitter* unit, int inNumSamples) {
    float* out = OUT(0);
    float freq = IN0(0);
    float jitterFracIn = IN0(2);
    size_t heapEffectiveSize = static_cast<size_t>(HEAP_MAX_SIZE / (12 * sc_log2(freq)));

    // Collect UGen state
    double phase = unit->mPhase;
    double inc = unit->mPhaseIncrement;
    float freqMul = unit->mFreqMul;

    // The maximum distance an impulse can be displaced
    int jitterWidth = static_cast<int>(jitterFracIn * unit->mImpulseHeap.maxSize);

    // Zero out the output buffer
    for (int xxn = 0; xxn < inNumSamples; xxn++) {
        out[xxn] = 0.f;
    }

    // Update the impulse table indices
    for (int xxn = 1; xxn < unit->mImpulseHeap.size; xxn++) {
        unit->mImpulseHeap.heap[xxn] -= inNumSamples;
    }

    // Retrieve impulses for this block
    while (heapPeek(&unit->mImpulseHeap) < inNumSamples && unit->mImpulseHeap.size > 1) {
        out[heapPop(&unit->mImpulseHeap)] = 1.f;
    }

    RGET
    for (int xxn = 0; xxn < inNumSamples; xxn++) {
        float impulseResult = testWrapPhase(inc, phase);
        if (impulseResult > 0.5f) {
            int idx = rgen.irand(jitterWidth) + xxn;
            if (idx < inNumSamples) {
                out[idx] = 1.f;
            } else if (unit->mImpulseHeap.size < heapEffectiveSize) {
                heapInsert(&unit->mImpulseHeap, idx);
            }
        }
        inc = freq * freqMul;
        phase += inc;
    }

    unit->mPhase = phase;
    unit->mPhaseIncrement = inc;
}

void ImpulseJitter_next_ak(ImpulseJitter* unit, int inNumSamples) {
    float* out = OUT(0);
    float freq = IN0(0);
    double off = IN0(1);
    float jitterFracIn = IN0(2);
    size_t heapEffectiveSize = static_cast<size_t>(HEAP_MAX_SIZE / (12 * sc_log2(freq)));
    
    // Collect UGen state
    double phase = unit->mPhase;
    double inc = unit->mPhaseIncrement;
    float freqMul = unit->mFreqMul;
    double prevOff = unit->mPhaseOffset;

    double offSlope = CALCSLOPE(off, prevOff);
    bool offChanged = offSlope != 0.f;

    // The maximum distance an impulse can be displaced
    int jitterWidth = static_cast<int>(jitterFracIn * unit->mImpulseHeap.maxSize);

    // Zero out the output buffer
    for (int xxn = 0; xxn < inNumSamples; xxn++) {
        out[xxn] = 0.f;
    }

    // Update the impulse table indices
    for (int xxn = 1; xxn < unit->mImpulseHeap.size; xxn++) {
        unit->mImpulseHeap.heap[xxn] -= inNumSamples;
    }

    // Retrieve impulses for this block
    while (heapPeek(&unit->mImpulseHeap) < inNumSamples && unit->mImpulseHeap.size > 1) {
        out[heapPop(&unit->mImpulseHeap)] = 1.f;
    }

    RGET
    for (int xxn = 0; xxn < inNumSamples; xxn++) {
        float impulseResult = testWrapPhase(inc, phase);
        if (impulseResult > 0.5f) {
            int idx = rgen.irand(jitterWidth) + xxn;
            if (idx < inNumSamples) {
                out[idx] = 1.f;
            } else if (unit->mImpulseHeap.size < heapEffectiveSize) {
                heapInsert(&unit->mImpulseHeap, idx);
            }
        }
        if (offChanged) {
            phase += offSlope;
            testWrapPhase(inc, phase);
        }
        inc = freq * freqMul;
        phase += inc;
    }

    unit->mPhase = phase;
    unit->mPhaseOffset = off;
    unit->mPhaseIncrement = inc;
}

void ImpulseJitter_next_ki(ImpulseJitter* unit, int inNumSamples) {
    float* out = OUT(0);
    double freq = IN0(0);
    double inc = freq * unit->mFreqMul;
    float jitterFracIn = IN0(2);
    size_t heapEffectiveSize = static_cast<size_t>(HEAP_MAX_SIZE / (12 * sc_log2(freq)));

    // Collect UGen state
    double phase = unit->mPhase;
    double prevInc = unit->mPhaseIncrement;
    
    double incSlope = CALCSLOPE(inc, prevInc);
    
    // The maximum distance an impulse can be displaced
    int jitterWidth = static_cast<int>(jitterFracIn * unit->mImpulseHeap.maxSize);

    // Zero out the output buffer
    for (int xxn = 0; xxn < inNumSamples; xxn++) {
        out[xxn] = 0.f;
    }

    // Update the impulse table indices
    for (int xxn = 1; xxn < unit->mImpulseHeap.size; xxn++) {
        unit->mImpulseHeap.heap[xxn] -= inNumSamples;
    }

    // Retrieve impulses for this block
    while (heapPeek(&unit->mImpulseHeap) < inNumSamples && unit->mImpulseHeap.size > 1) {
        out[heapPop(&unit->mImpulseHeap)] = 1.f;
    }

    RGET
    for (int xxn = 0; xxn < inNumSamples; xxn++) {
        float impulseResult = testWrapPhase(prevInc, phase);
        if (impulseResult > 0.5f) {
            int idx = rgen.irand(jitterWidth) + xxn;
            if (idx < inNumSamples) {
                out[idx] = 1.f;
            } else if (unit->mImpulseHeap.size < heapEffectiveSize) {
                heapInsert(&unit->mImpulseHeap, idx);
            }
        }
        prevInc += incSlope;
        phase += prevInc;
    }

    unit->mPhase = phase;
    unit->mPhaseIncrement = inc;
}

void ImpulseJitter_next_kk(ImpulseJitter* unit, int inNumSamples) {
    float* out = OUT(0);
    double freq = IN0(0);
    double inc = freq * unit->mFreqMul;
    double off = IN0(1);
    float jitterFracIn = IN0(2);
    size_t heapEffectiveSize = static_cast<size_t>(HEAP_MAX_SIZE / (12 * sc_log2(freq)));

    // Collect UGen state
    double phase = unit->mPhase;
    double prevInc = unit->mPhaseIncrement;
    double prevOff = unit->mPhaseOffset;
    
    double incSlope = CALCSLOPE(inc, prevInc);
    double phaseSlope = CALCSLOPE(off, prevOff);
    bool phOffChanged = phaseSlope != 0.f;

    // The maximum distance an impulse can be displaced
    int jitterWidth = static_cast<int>(jitterFracIn * unit->mImpulseHeap.maxSize);

    // Zero out the output buffer
    for (int xxn = 0; xxn < inNumSamples; xxn++) {
        out[xxn] = 0.f;
    }

    // Update the impulse table indices
    for (int xxn = 1; xxn < unit->mImpulseHeap.size; xxn++) {
        unit->mImpulseHeap.heap[xxn] -= inNumSamples;
    }

    // Retrieve impulses for this block
    while (heapPeek(&unit->mImpulseHeap) < inNumSamples && unit->mImpulseHeap.size > 1) {
        out[heapPop(&unit->mImpulseHeap)] = 1.f;
    }

    RGET
    for (int xxn = 0; xxn < inNumSamples; xxn++) {
        float impulseResult = testWrapPhase(prevInc, phase);
        if (impulseResult > 0.5f) {
            int idx = rgen.irand(jitterWidth) + xxn;
            if (idx < inNumSamples) {
                out[idx] = 1.f;
            } else if (unit->mImpulseHeap.size < heapEffectiveSize) {
                heapInsert(&unit->mImpulseHeap, idx);
            }
        }
        if (phOffChanged) {
            phase += phaseSlope;
            testWrapPhase(prevInc, phase);
        }
        prevInc += incSlope;
        phase += prevInc;
    }

    unit->mPhase = phase;
    unit->mPhaseOffset = off;
    unit->mPhaseIncrement = inc;
}

// Construct the ImpulseJitter
void ImpulseJitter_Ctor(ImpulseJitter* unit) {
    unit->mPhaseIncrement = IN0(0) * unit->mFreqMul;
    unit->mPhaseOffset = IN0(1);
    unit->mFreqMul = static_cast<float>(unit->mRate->mSampleDur);
    unit->mImpulseHeap.maxSize = HEAP_MAX_SIZE;  // hard coded for now
    unit->mImpulseHeap.size = 1;
    unit->mImpulseHeap.heap = (int*)RTAlloc(unit->mWorld, HEAP_MAX_SIZE * sizeof(int));

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
            func = (UnitCalcFunc)ImpulseJitter_next_ai;
            //printf("Calc function set to ai\n");
            break;
        case calc_BufRate:
            func = (UnitCalcFunc)ImpulseJitter_next_ak;
            //printf("Calc function set to ak\n");
            break;
        case calc_FullRate:
            func = (UnitCalcFunc)ImpulseJitter_next_aa;
            //printf("Calc function set to aa\n");
            break;
        }
        break;
    case calc_BufRate:
        if (INRATE(1) == calc_ScalarRate) {
            func = (UnitCalcFunc)ImpulseJitter_next_ki;
            //printf("Calc function set to ki\n");
        } else {
            func = (UnitCalcFunc)ImpulseJitter_next_kk;
            //printf("Calc function set to kk\n");
        }
        break;
    case calc_ScalarRate:
        if (INRATE(1) == calc_ScalarRate) {
            func = (UnitCalcFunc)ImpulseJitter_next_ki;
            //printf("Calc function set to ki\n");
        } else {
            func = (UnitCalcFunc)ImpulseJitter_next_kk;
            //printf("Calc function set to kk\n");
        }
        break;
    }
    unit->mCalcFunc = func;
    func(unit, 1);

    unit->mPhase = initPhase;
    unit->mPhaseOffset = initOff;
    unit->mPhaseIncrement = initInc;
}

void ImpulseJitter_Dtor(ImpulseJitter* unit) {
    RTFree(unit->mWorld, unit->mImpulseHeap.heap);
}