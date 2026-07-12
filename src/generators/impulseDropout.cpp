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

FlexPlugins::ImpulseDropout::ImpulseDropout() {
    mPhaseIncrement = in0(0) * mFreqMul;
    mPhaseOffset = in0(1);
    mFreqMul = static_cast<float>(mRate->mSampleDur);

    double initOff = mPhaseOffset;
    double initInc = mPhaseIncrement;
    double initPhase = sc_wrap(initOff, 0.0, 1.0);

    // Initial phase offset of 0 means output of 1 on first sample.
    // Set phase to wrap point to trigger impulse on first sample
    if (initPhase == 0.0 && initInc >= 0.0) {
        initPhase = 1.0; // positive frequency trigger/wrap position
    }
    mPhase = initPhase;

    UnitCalcFunc func;
    switch (inRate(0)) {
    case calc_FullRate:
        switch (inRate(1)) {
        case calc_ScalarRate:
            set_calc_function<ImpulseDropout, &ImpulseDropout::next_ai>();
            next_ai(1);
            break;
        case calc_BufRate:
            set_calc_function<ImpulseDropout, &ImpulseDropout::next_ak>();
            next_ak(1);
            break;
        case calc_FullRate:
            set_calc_function<ImpulseDropout, &ImpulseDropout::next_aa>();
            next_aa(1);
            break;
        }
        break;
    case calc_BufRate:
    case calc_ScalarRate:
        if (inRate(1) == calc_ScalarRate) {
            set_calc_function<ImpulseDropout, &ImpulseDropout::next_ki>();
            next_ki(1);
        } else {
            set_calc_function<ImpulseDropout, &ImpulseDropout::next_kk>();
            next_ki(1);
        }
        break;
    }

    mPhase = initPhase;
    mPhaseOffset = initOff;
    mPhaseIncrement = initInc;
}

void FlexPlugins::ImpulseDropout::next_aa(int inNumSamples) {
    float* outBuf = out(0);
    const float* freqIn = in(0);
    const float* offIn = in(1);
    float dropProbIn = in0(2);
    
    // Collect UGen state
    double phase = mPhase;
    double inc = mPhaseIncrement;
    float freqMul = mFreqMul;
    double prevOff = mPhaseOffset;
    
    RGen* rgen = mParent->mRGen;
    uint32 s1 = rgen->s1;                                                                                               \
    uint32 s2 = rgen->s2;                                                                                               \
    uint32 s3 = rgen->s3;

    for (int i = 0; i < inNumSamples; i++) {
        float impulseResult = testWrapPhase(inc, phase);
        // Drop the impulse if necessary
        if (impulseResult > 0.5f && rgen->frand() < dropProbIn) {
            impulseResult = 0.f;
        }
        double off = static_cast<double>(offIn[i]);
        double offInc = off - prevOff;
        phase += offInc;
        testWrapPhase(inc, phase);
        inc = freqIn[i] * freqMul;
        outBuf[i] = impulseResult;
        phase += inc;
        prevOff = off;
    }

    mPhase = phase;
    mPhaseOffset = prevOff;
    mPhaseIncrement = inc;
}

void FlexPlugins::ImpulseDropout::next_ai(int inNumSamples) {
    float* outBuf = out(0);
    float freqIn = in0(0);
    float dropProbIn = in0(2);

    // Collect UGen state
    double phase = mPhase;
    double inc = mPhaseIncrement;
    float freqMul = mFreqMul;

    RGen* rgen = mParent->mRGen;
    uint32 s1 = rgen->s1;                                                                                               \
    uint32 s2 = rgen->s2;                                                                                               \
    uint32 s3 = rgen->s3;
    for (int i = 0; i < inNumSamples; i++) {
        float impulseResult = testWrapPhase(inc, phase);
        // Drop the impulse if necessary
        if (impulseResult > 0.5f && rgen->frand() < dropProbIn) {
            impulseResult = 0.f;
        }
        inc = freqIn * freqMul;
        outBuf[i] = impulseResult;
        phase += inc;
    }

    mPhase = phase;
    mPhaseIncrement = inc;
}

void FlexPlugins::ImpulseDropout::next_ak(int inNumSamples) {
    float* outBuf = out(0);
    float freqIn = in0(0);
    double off = in0(1);
    float dropProbIn = in0(2);
    
    // Collect UGen state
    double phase = mPhase;
    double inc = mPhaseIncrement;
    float freqMul = mFreqMul;
    double prevOff = mPhaseOffset;

    double offSlope = calcSlope(off, prevOff);
    bool offChanged = offSlope != 0.f;

    RGen* rgen = mParent->mRGen;
    uint32 s1 = rgen->s1;                                                                                               \
    uint32 s2 = rgen->s2;                                                                                               \
    uint32 s3 = rgen->s3;
    for (int i = 0; i < inNumSamples; i++) {
        float impulseResult = testWrapPhase(inc, phase);
        // Drop the impulse if necessary
        if (impulseResult > 0.5f && rgen->frand() < dropProbIn) {
            impulseResult = 0.f;
        }
        if (offChanged) {
            phase += offSlope;
            testWrapPhase(inc, phase);
        }
        inc = freqIn * freqMul;
        outBuf[i] = impulseResult;
        phase += inc;
    }

    mPhase = phase;
    mPhaseOffset = off;
    mPhaseIncrement = inc;
}

void FlexPlugins::ImpulseDropout::next_ki(int inNumSamples) {
    float* outBuf = out(0);
    double inc = in0(0) * mFreqMul;
    float dropProbIn = in0(2);

    // Collect UGen state
    double phase = mPhase;
    double prevInc = mPhaseIncrement;
    
    double incSlope = calcSlope(inc, prevInc);
    
    RGen* rgen = mParent->mRGen;
    uint32 s1 = rgen->s1;                                                                                               \
    uint32 s2 = rgen->s2;                                                                                               \
    uint32 s3 = rgen->s3;
    for (int i = 0; i < inNumSamples; i++) {
        float impulseResult = testWrapPhase(prevInc, phase);
        // Drop the impulse if necessary
        if (impulseResult > 0.5f && rgen->frand() < dropProbIn) {
            impulseResult = 0.f;
        }
        outBuf[i] = impulseResult;
        prevInc += incSlope;
        phase += prevInc;
    }

    mPhase = phase;
    mPhaseIncrement = inc;
}

void FlexPlugins::ImpulseDropout::next_kk(int inNumSamples) {
    float* outBuf = out(0);
    double inc = in0(0) * mFreqMul;
    double off = in0(1);
    float dropProbIn = in0(2);

    // Collect UGen state
    double phase = mPhase;
    double prevInc = mPhaseIncrement;
    double prevOff = mPhaseOffset;
    
    double incSlope = calcSlope(inc, prevInc);
    double phaseSlope = calcSlope(off, prevOff);
    bool phOffChanged = phaseSlope != 0.f;

    RGen* rgen = mParent->mRGen;
    uint32 s1 = rgen->s1;                                                                                               \
    uint32 s2 = rgen->s2;                                                                                               \
    uint32 s3 = rgen->s3;
    for (int i = 0; i < inNumSamples; i++) {
        float impulseResult = testWrapPhase(prevInc, phase);
        // Drop the impulse if necessary
        if (impulseResult > 0.5f && rgen->frand() < dropProbIn) {
            impulseResult = 0.f;
        }
        if (phOffChanged) {
            phase += phaseSlope;
            testWrapPhase(prevInc, phase);
        }
        outBuf[i] = impulseResult;
        prevInc += incSlope;
        phase += prevInc;
    }

    mPhase = phase;
    mPhaseOffset = off;
    mPhaseIncrement = inc;
}
