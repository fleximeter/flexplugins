/*
File: impulseJitter.hpp
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

#pragma once
#include "SC_PlugIn.h"
#include "arrayheap.hpp"

#define HEAP_MAX_SIZE 1024
// Represents an ImpulseJitter UGen.
struct ImpulseJitter : public Unit {
    double mPhase, mPhaseOffset, mPhaseIncrement;
    float mFreqMul;
    IntMinHeap mImpulseHeap;
};

void ImpulseJitter_Ctor(ImpulseJitter* unit);
void ImpulseJitter_Dtor(ImpulseJitter* unit);
void ImpulseJitter_next_aa(ImpulseJitter* unit, int inNumSamples);
void ImpulseJitter_next_ai(ImpulseJitter* unit, int inNumSamples);
void ImpulseJitter_next_ak(ImpulseJitter* unit, int inNumSamples);
void ImpulseJitter_next_ki(ImpulseJitter* unit, int inNumSamples);
void ImpulseJitter_next_kk(ImpulseJitter* unit, int inNumSamples);
