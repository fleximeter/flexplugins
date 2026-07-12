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
extern InterfaceTable *ft;

FlexPlugins::FIR::FIR() {
    m_z = static_cast<float*>(RTAlloc(mWorld, fullBufferSize() * sizeof(float)));
    for (size_t i = 0; i < fullBufferSize(); i++) {
        m_z[i] = 0.f;
    }
    set_calc_function<FlexPlugins::FIR, &FlexPlugins::FIR::next>();
    next(1);
}

FlexPlugins::FIR::~FIR() {
    if (m_z) RTFree(mWorld, m_z);
}

void FlexPlugins::FIR::next(int nSamples) {
    size_t numCoefs = static_cast<size_t>(mNumInputs - 1);
    numCoefs = sc_clip(numCoefs, 0, static_cast<size_t>(fullBufferSize()));
    const float *inBuf = in(0);
    float *outBuf = out(0);
    for (size_t i = 0; i < nSamples; i++) {
        float convResult = 0.f;
        // unit delay
        for (size_t j = fullBufferSize() - 1; j > 0; j--) {
            m_z[j] = m_z[j-1];
        }
        m_z[0] = inBuf[i];
        // convolve
        for (size_t k = 0; k < numCoefs; k++) {
            convResult += in0(1+k) * m_z[k];
        }
        outBuf[i] = convResult;
    }
}