/*
File: fir.cpp
Author: Jeff Martin

Description:
A raw FIR filter

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

#include "fir.hpp"
#include "SC_Unit.h"
extern InterfaceTable *ft;

void FIR_Ctor(FIR *unit) {
    unit->m_z = static_cast<float*>(RTAlloc(unit->mWorld, FULLBUFLENGTH * sizeof(float)));
    for (size_t i = 0; i < FULLBUFLENGTH; i++) {
        unit->m_z[i] = 0.f;
    }
    SETCALC(FIR_next);
    OUT0(0) = 0;
}

void FIR_Dtor(FIR *unit) {
    if (unit->m_z) RTFree(unit->mWorld, unit->m_z);
}

void FIR_next(FIR *unit, int inNumSamples) {
    size_t numCoefs = static_cast<size_t>(unit->mNumInputs - 1);
    numCoefs = sc_clip(numCoefs, 0, static_cast<size_t>(FULLBUFLENGTH));
    const float *inBuf = IN(0);
    float *outBuf = OUT(0);
    for (size_t xxi = 0; xxi < inNumSamples; xxi++) {
        float convResult = 0.f;
        // unit delay
        for (size_t xxj = FULLBUFLENGTH - 1; xxj > 0; xxj--) {
            unit->m_z[xxj] = unit->m_z[xxj-1];
        }
        unit->m_z[0] = inBuf[xxi];
        // convolve
        for (size_t xxk = 0; xxk < numCoefs; xxk++) {
            convResult += IN0(1+xxk) * unit->m_z[xxk];
        }
        outBuf[xxi] = convResult;
    }
}