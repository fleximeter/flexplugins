/*
File: impulseDropout.hpp
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

#pragma once
#include "SC_PlugIn.h"
#define HEAP_MAX_SIZE 1024

// Represents an ImpulseDropout UGen.
struct ImpulseDropout : public Unit {
    double mPhase, mPhaseOffset, mPhaseIncrement;
    float mFreqMul;
};

void ImpulseDropout_Ctor(ImpulseDropout* unit);
void ImpulseDropout_next_aa(ImpulseDropout* unit, int inNumSamples);
void ImpulseDropout_next_ai(ImpulseDropout* unit, int inNumSamples);
void ImpulseDropout_next_ak(ImpulseDropout* unit, int inNumSamples);
void ImpulseDropout_next_ki(ImpulseDropout* unit, int inNumSamples);
void ImpulseDropout_next_kk(ImpulseDropout* unit, int inNumSamples);
