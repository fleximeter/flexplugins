/*
File: loopPhasor.cpp
Author: Jeff Martin

Description:
The LoopPhasor UGen

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

#include "loopPhasor.hpp"
#include "SC_Rate.h"
extern InterfaceTable *ft;

// Construct the LoopPhasor
FlexPlugins::LoopPhasor::LoopPhasor() {
    // Set the calculation function 
    if (inRate(2) == calc_FullRate) {
        if (inRate(0) == calc_FullRate) {
            set_calc_function<LoopPhasor, &LoopPhasor::next_aa>();
            Print("Set aa\n");
            next_aa(1);
        } else {
            Print("Set ak\n");
            set_calc_function<LoopPhasor, &LoopPhasor::next_ak>();
            next_ak(1);
        }
    } else {
        Print("Set kk\n");
        set_calc_function<LoopPhasor, &LoopPhasor::next_k>();
        next_k(1);
    }

    // Initialize the triggers
    m_prevTriggerStart = in0(0);
    m_prevTriggerFinish = in0(1);
    m_loopState = false;

    // Initialize the output
    m_level = in0(3);
}

// all parameters are control rate
void FlexPlugins::LoopPhasor::next_k(int inNumSamples) {
    // Pointer to output array
    float* outBuf = out(0);

    // Get new parameters of the LoopPhasor
    const float triggerReturnToStart = in0(0);
    const float triggerFinish = in0(1);
    const float rate = in0(2);
    const float startPosition = in0(3);
    const float endPosition = in0(4);
    const float loopStart = in0(5);
    const float loopEnd = in0(6);

    // Handle trigger return to start
    if (m_prevTriggerStart <= 0.f && triggerReturnToStart > 0.f) {
        m_level = startPosition;
    }

    // Handle trigger finish. This just flips the loop state.
    if (m_prevTriggerFinish <= 0.f && triggerFinish > 0.f) {
        m_triggerFinishState = !m_triggerFinishState;
    }

    // Compute output block
    for (int i = 0; i < inNumSamples; i++) {
        if (!m_triggerFinishState) {
            if (m_level >= loopStart) {
                m_loopState = true;
            }
            if (m_loopState) {
                m_level = sc_wrap(m_level, loopStart, loopEnd);
            }
        }

        else {
            m_loopState = false;
            m_level = sc_clip(m_level, startPosition, endPosition);
        }
        
        outBuf[i] = static_cast<float>(m_level);
        m_level += rate;
    }

    // Update the state of the LoopPhasor
    m_prevTriggerStart = triggerReturnToStart;
    m_prevTriggerFinish = triggerFinish;
}

// rate parameter is audio rate
void FlexPlugins::LoopPhasor::next_ak(int inNumSamples) {
    // Pointer to output array
    float* outBuf = out(0);

    // Get new parameters of the LoopPhasor
    const float triggerReturnToStart = in0(0);
    const float triggerFinish = in0(1);
    const float *rate = in(2);
    const float startPosition = in0(3);
    const float endPosition = in0(4);
    const float loopStart = in0(5);
    const float loopEnd = in0(6);

    // Handle trigger return to start
    if (m_prevTriggerStart <= 0.f && triggerReturnToStart > 0.f) {
        m_level = startPosition;
    }

    // Handle trigger finish. This just flips the finish trigger.
    if (m_prevTriggerFinish <= 0.f && triggerFinish > 0.f) {
        m_triggerFinishState = !(m_triggerFinishState);
    }

    // Compute output block
    for (int i = 0; i < inNumSamples; i++) {
        if (!m_triggerFinishState) {
            if (m_level >= loopStart) {
                m_loopState = true;
            }
            if (m_loopState) {
                m_level = sc_wrap(m_level, loopStart, loopEnd);
            }
        }

        else {
            m_level = sc_clip(m_level, startPosition, endPosition);
        }

        outBuf[i] = static_cast<float>(m_level);
        m_level += rate[i];
    }
    m_prevTriggerStart = triggerReturnToStart;
    m_prevTriggerFinish = triggerFinish;
}

// first two parameters are audio rate
void FlexPlugins::LoopPhasor::next_aa(int inNumSamples) {
    float* outBuf = out(0);

    // Get new parameters of the LoopPhasor
    const float *triggerReturnToStart = in(0);
    const float triggerFinish = in0(1);
    const float *rate = in(2);
    const float startPosition = in0(3);
    const float endPosition = in0(4);
    const float loopStart = in0(5);
    const float loopEnd = in0(6);

    // Handle trigger finish. This just flips the finish trigger.
    if (m_prevTriggerFinish <= 0.f && triggerFinish > 0.f) {
        m_triggerFinishState = !(m_triggerFinishState);
    }

    // Compute output block
    for (int i = 0; i < inNumSamples; i++) {
        // Handle trigger return to start
        if (m_prevTriggerStart <= 0.f && triggerReturnToStart[i] > 0.f) {
            float frac = 1.f - m_prevTriggerStart / (triggerReturnToStart[i] - m_prevTriggerStart);
            m_level = startPosition + frac * rate[i];
        }

        if (!m_triggerFinishState) {
            if (m_level >= loopStart) {
                m_loopState = true;
            }
            if (m_loopState) {
                m_level = sc_wrap(m_level, loopStart, loopEnd);
            }
        }

        else {
            m_level = sc_clip(m_level, startPosition, endPosition);
        }
        
        outBuf[i] = static_cast<float>(m_level);
        m_level += rate[i];
        m_prevTriggerStart = triggerReturnToStart[i];
    }
    m_prevTriggerFinish = triggerFinish;
}
