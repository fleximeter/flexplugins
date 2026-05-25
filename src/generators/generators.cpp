/*
File: generators.cpp
Author: Jeff Martin

Description:
This is a SuperColllider UGen based on the Phasor class.
It allows setting loop start and end points for playing samples.
This is useful for mimicking the functionality of commercial sampler synthesizers.

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

#include "SC_PlugIn.h"

static InterfaceTable *ft;

// Represents a LoopPhasor UGen.
struct LoopPhasor : public Unit {
    double m_level;             // LoopPhasor output level (position of the phasor between `start` and `end`)
    float m_prevTriggerStart;   // previous value of trigger to return to start position
    float m_prevTriggerFinish;  // previous value of trigger to finish
    bool m_triggerFinishState;  // current state of finish trigger (true - finish; false - continue looping)
};

static void LoopPhasor_next_aa(LoopPhasor* unit, int inNumSamples);
static void LoopPhasor_next_ak(LoopPhasor* unit, int inNumSamples);
static void LoopPhasor_next_kk(LoopPhasor* unit, int inNumSamples);
static void LoopPhasor_Ctor(LoopPhasor* unit);

// Construct the LoopPhasor
void LoopPhasor_Ctor(LoopPhasor* unit) {
    // Set the calculation function 
    if (unit->mCalcRate == calc_FullRate) {
        if (INRATE(0) == calc_FullRate) {
            if (INRATE(1) == calc_FullRate) {
                SETCALC(LoopPhasor_next_aa);
            } else {
                SETCALC(LoopPhasor_next_ak);
            }
        } else {
            SETCALC(LoopPhasor_next_kk);
        }
    } else {
        SETCALC(LoopPhasor_next_ak);
    }

    // Initialize the triggers
    unit->m_prevTriggerStart = IN0(0);
    unit->m_prevTriggerFinish = IN0(1);
    unit->m_triggerFinishState = false;

    // Initialize the output
    unit->m_level = IN0(3);
    ZOUT0(0) = static_cast<float>(unit->m_level);
}

// Calculates samples for a LoopPhasor.kr UGen
void LoopPhasor_next_kk(LoopPhasor* unit, int inNumSamples) {
    // Pointer to output array
    float* out = OUT(0);

    // Get new parameters of the LoopPhasor
    float triggerReturnToStart = IN0(0);
    float triggerFinish = IN0(1);
    double rate = IN0(2);
    double startPosition = IN0(3);
    double endPosition = IN0(4);
    double loopStart = IN0(5);
    double loopEnd = IN0(6);

    // Get current state of the LoopPhasor
    float previousTriggerReturnToStart = unit->m_prevTriggerStart;  // trigger return to start
    float previousTriggerFinish = unit->m_prevTriggerFinish;  // trigger finish
    double level = unit->m_level;

    // Handle trigger return to start
    if (previousTriggerReturnToStart <= 0.f && triggerReturnToStart > 0.f) {
        level = startPosition;
    }

    // Handle trigger finish. This just flips the finish trigger.
    if (previousTriggerFinish <= 0.f && triggerFinish > 0.f) {
        unit->m_triggerFinishState = !(unit->m_triggerFinishState);
    }

    // Compute output block
    for (int xxn = 0; xxn < inNumSamples; xxn++) {
        // If we haven't triggered completion
        if (!unit->m_triggerFinishState) {
            // If we're inside the looping part of the LoopPhasor
            if (level >= loopStart && level <= loopEnd) {
                level = sc_wrap(level, loopStart, loopEnd);
            } else {
                level = sc_wrap(level, startPosition, endPosition);
            }
        }

        // Otherwise we wrap up
        else {
            level = sc_max(level, startPosition);
            level = sc_min(level, endPosition);
        }
        
        out[xxn] = static_cast<float>(level);
        level += rate;
    }

    // Update the state of the LoopPhasor
    unit->m_prevTriggerStart = triggerReturnToStart;
    unit->m_prevTriggerFinish = triggerFinish;
    unit->m_level = level;
}

// Calculates samples for a LoopPhasor.ar ugen with .kr parameters
void LoopPhasor_next_ak(LoopPhasor* unit, int inNumSamples) {
    // Pointer to output array
    float* out = OUT(0);

    // Get new parameters of the LoopPhasor
    float *triggerReturnToStart = IN(0);
    float *triggerFinish = IN(1);
    double rate = IN0(2);
    double startPosition = IN0(3);
    double endPosition = IN0(4);
    double loopStart = IN0(5);
    double loopEnd = IN0(6);

    // Get current state of the LoopPhasor
    float previousTriggerReturnToStart = unit->m_prevTriggerStart;
    float previousTriggerFinish = unit->m_prevTriggerFinish;
    double level = unit->m_level;

    // Compute output block
    for (int xxn = 0; xxn < inNumSamples; xxn++) {
        // If we reset to start
        if (previousTriggerReturnToStart <= 0.f && triggerReturnToStart[xxn] > 0.f) {
            float frac = 1.f - previousTriggerReturnToStart / (triggerReturnToStart[xxn] - previousTriggerReturnToStart);
            level = startPosition + frac * rate;
        }

        // Handle trigger finish. This just flips the finish trigger.
        if (previousTriggerFinish <= 0.f && triggerFinish[xxn] > 0.f) {
            unit->m_triggerFinishState = !(unit->m_triggerFinishState);
        }

        // Wrapping: if we haven't triggered completion
        if (!unit->m_triggerFinishState) {
            // if we're inside the looping part of the LoopPhasor
            if (level >= loopStart && level <= loopEnd) {
                level = sc_wrap(level, loopStart, loopEnd);
            } else {
                level = sc_wrap(level, startPosition, endPosition);
            }
        }

        // Wrapping: if we have triggered completion
        else {
            level = sc_max(level, startPosition);
            level = sc_min(level, endPosition);
        }

        out[xxn] = static_cast<float>(level);
        level += rate;
        previousTriggerReturnToStart = triggerReturnToStart[xxn];
        previousTriggerFinish = triggerFinish[xxn];
    }

    // update the state of the LoopPhasor
    unit->m_prevTriggerStart = previousTriggerReturnToStart;
    unit->m_prevTriggerFinish = previousTriggerFinish;
    unit->m_level = level;
}

// Calculates samples for a LoopPhasor.ar UGen with .ar parameters
void LoopPhasor_next_aa(LoopPhasor* unit, int inNumSamples) {
    float* out = OUT(0);

    // Get new parameters of the LoopPhasor
    float *triggerReturnToStart = IN(0);
    float *triggerFinish = IN(1);
    float *rate = IN(2);
    double startPosition = IN0(3);
    double endPosition = IN0(4);
    double loopStart = IN0(5);
    double loopEnd = IN0(6);

    // Get current state of the LoopPhasor
    float previousTriggerReturnToStart = unit->m_prevTriggerStart;
    float previousTriggerFinish = unit->m_prevTriggerFinish;
    double level = unit->m_level;

    float *in = triggerReturnToStart;
    float previn = previousTriggerReturnToStart;

    // Compute output block
    for (int xxn = 0; xxn < inNumSamples; xxn++) {
        // Handle trigger return to start
        if (previousTriggerReturnToStart <= 0.f && triggerReturnToStart[xxn] > 0.f) {
            float frac = 1.f - previousTriggerReturnToStart / (triggerReturnToStart[xxn] - previousTriggerReturnToStart);
            level = startPosition + frac * rate[xxn];
        }

        // Handle trigger finish. This just flips the finish trigger.
        if (previousTriggerFinish <= 0.f && triggerFinish[xxn] > 0.f) {
            unit->m_triggerFinishState = !(unit->m_triggerFinishState);
        }

        // Wrapping: if we haven't triggered completion
        if (!unit->m_triggerFinishState) {
            // If we're inside the looping part of the LoopPhasor
            if (level >= loopStart && level <= loopEnd) {
                level = sc_wrap(level, loopStart, loopEnd);
            } else {
                level = sc_wrap(level, startPosition, endPosition);
            }
        }

        // Wrapping: if we have triggered completion
        else {
            level = sc_max(level, startPosition);
            level = sc_min(level, endPosition);
        }
        
        out[xxn] = static_cast<float>(level);
        level += rate[xxn];
        previousTriggerReturnToStart = triggerReturnToStart[xxn];
        previousTriggerFinish = triggerFinish[xxn];
    }
    
    // update the state of the LoopPhasor at the end of the calculation block
    unit->m_prevTriggerStart = previousTriggerReturnToStart;
    unit->m_prevTriggerFinish = previousTriggerFinish;
    unit->m_level = level;
}

PluginLoad(LoopPhasor) {
    ft = inTable;
    DefineSimpleUnit(LoopPhasor);
}
