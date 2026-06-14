/*
File: pvCFreeze.hpp
Author: Jeff Martin

Description:
Jean-François Charles spectral freeze

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

#pragma once

#include "SC_Unit.h"

struct PV_CFreeze : public Unit {
    int mNumBins;    // The number of FFT bins
    int mNumFrames;  // The number of candidate FFT frames to maintain
    float *mMags;    // The 2D array of FFT mags
    float *mDc;      // The 1D array of FFT DC values
    float *mNyq;     // The 1D array of FFT Nyquist values
    float *mPhase;   // The most recent phase array
    float *mPhaseDiffs;  // The 2D array of FFT phase differences
    size_t mWritePtr;   // The write pointer
};

void PV_CFreeze_next(PV_CFreeze *unit, int inNumSamples);
void PV_CFreeze_Ctor(PV_CFreeze *unit);
void PV_CFreeze_Dtor(PV_CFreeze *unit);