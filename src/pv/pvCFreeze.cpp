/*
File: pvCFreeze.cpp
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

#include "pvCFreeze.hpp"
#include "FFT_UGens.h"

extern InterfaceTable *ft;

void FlexPlugins::PV_CFreeze::next(int inNumSamples) {
    PV_CFreeze *unit = this;
    PV_GET_BUF
    float freezeState = in0(1);
    // allocate the buffers
    if (!mMags) {
        // MxN where N is num bins, and M is num frames. Acts as a circular buffer.
        mMags = (float*)RTAlloc(mWorld, numbins * sizeof(float) * mNumFrames);
        // M (num frames)
        mDc = (float*)RTAlloc(mWorld, sizeof(float) * mNumFrames);
        // M (num frames)
        mNyq = (float*)RTAlloc(mWorld, sizeof(float) * mNumFrames);
        // N (num bins)
        mPhase = (float*)RTAlloc(mWorld, numbins * sizeof(float));
        // MxN where N is num bins, and M is num frames.
        // Acts as a circular buffer corresponding to mMags.
        mPhaseDiffs = (float*)RTAlloc(mWorld, numbins * sizeof(float) * mNumFrames);
        ClearFFTUnitIfMemFailed(mMags);
        ClearFFTUnitIfMemFailed(mDc);
        ClearFFTUnitIfMemFailed(mNyq);
        ClearFFTUnitIfMemFailed(mPhase);
        ClearFFTUnitIfMemFailed(mPhaseDiffs);
        mNumBins = numbins;
        mWritePtr = 0;
    } else if (numbins != mNumBins) {
        // Cannot allow the FFT size to change
        return;
    }

    SCPolarBuf *p = ToPolarApx(buf);

    if (freezeState > 0.f) {
        RGET
        // Pull random DC and nyquist magnitudes
        p->dc = mDc[rgen.irand(mNumFrames)];
        p->nyq = mNyq[rgen.irand(mNumFrames)];
        for (int k = 0; k < mNumBins; k++) {
            // For each bin, grab a random magnitude and phase diff pair
            int idx = rgen.irand(mNumFrames);
            idx = idx * mNumBins + k;
            p->bin[k].mag = mMags[idx];
            mPhase[k] = sc_wrap(mPhase[k] + mPhaseDiffs[idx], 0.f, static_cast<float>(twopi));
            p->bin[k].phase = mPhase[k];
        }
    } else {
        // We're writing to a circular buffer, so pull the current magnitude and phase diff arrays
        float *currentMagArr = mMags + (mWritePtr * mNumBins);
        float *currentPhaseDiffArr = mPhaseDiffs + (mWritePtr * mNumBins);
        for (int k = 0; k < numbins; k++) {
            currentMagArr[k] = p->bin[k].mag;
            currentPhaseDiffArr[k] = sc_wrap(p->bin[k].phase - mPhase[k], 0.f, static_cast<float>(twopi));
            mPhase[k] = p->bin[k].phase;
        }
        mDc[mWritePtr] = p->dc;
        mNyq[mWritePtr] = p->nyq;
        mWritePtr++;
        mWritePtr %= mNumFrames;
    }
}

FlexPlugins::PV_CFreeze::PV_CFreeze() {
    mMags = nullptr;
    mDc = nullptr;
    mNyq = nullptr;
    mPhase = nullptr;
    mPhaseDiffs = nullptr;
    int numFrames = static_cast<int>(in0(2));
    // prevent the user from doing something nuts
    mNumFrames = sc_clip(numFrames, 1, 64);
    set_calc_function<PV_CFreeze, &PV_CFreeze::next>();
    next(1);
}

FlexPlugins::PV_CFreeze::~PV_CFreeze() {
    if (mMags) {
        RTFree(mWorld, mMags);
        mMags = nullptr;
    }
    if (mDc) {
        RTFree(mWorld, mDc);
        mDc = nullptr;
    }
    if (mNyq) {
        RTFree(mWorld, mNyq);
        mNyq = nullptr;
    }
    if (mPhase) {
        RTFree(mWorld, mPhase);
        mPhase = nullptr;
    }
    if (mPhaseDiffs) {
        RTFree(mWorld, mPhaseDiffs);
        mPhaseDiffs = nullptr;
    }
}
