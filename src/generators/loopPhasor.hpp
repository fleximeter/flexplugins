/*
File: loopPhasor.hpp
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

#pragma once
#include "SC_PlugIn.hpp"

namespace FlexPlugins {
    // Represents a LoopPhasor UGen.
    class LoopPhasor : public SCUnit {
    public:
        LoopPhasor();

    private:
        void next_ak(int inNumSamples);
        void next_aa(int inNumSamples);
        void next_k(int inNumSamples);
        float m_level;              // LoopPhasor output level (position of the phasor between `start` and `end`)
        float m_prevTriggerStart;   // previous value of trigger to return to start position
        float m_prevTriggerFinish;  // previous value of trigger to finish
        bool m_triggerFinishState;  // current state of finish trigger (true - finish; false - continue looping)
        float m_loopState;          // Tracks if we are currently looping
    };
}
