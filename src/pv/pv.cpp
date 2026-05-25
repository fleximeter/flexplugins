/*
File: pv.cpp
Author: Jeff Martin

Description:


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

#include "SC_Constants.h"
#include "SC_InterfaceTable.h"
#include "FFT_UGens.h"
#include "SC_Unit.h"

InterfaceTable *ft;

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

struct PV_MagSqueeze : public Unit {};

struct PV_MagMirror : public Unit {};

struct PV_MagXFade : public Unit {};

static void PV_CFreeze_next(PV_CFreeze *unit, int inNumSamples) {
    PV_GET_BUF
    float freezeState = IN0(1);
    // allocate the buffers
    if (!unit->mMags) {
        // MxN where N is num bins, and M is num frames. Acts as a circular buffer.
        unit->mMags = (float*)RTAlloc(unit->mWorld, numbins * sizeof(float) * unit->mNumFrames);
        // M (num frames)
        unit->mDc = (float*)RTAlloc(unit->mWorld, sizeof(float) * unit->mNumFrames);
        // M (num frames)
        unit->mNyq = (float*)RTAlloc(unit->mWorld, sizeof(float) * unit->mNumFrames);
        // N (num bins)
        unit->mPhase = (float*)RTAlloc(unit->mWorld, numbins * sizeof(float));
        // MxN where N is num bins, and M is num frames.
        // Acts as a circular buffer corresponding to unit->mMags.
        unit->mPhaseDiffs = (float*)RTAlloc(unit->mWorld, numbins * sizeof(float) * unit->mNumFrames);
        ClearFFTUnitIfMemFailed(unit->mMags);
        ClearFFTUnitIfMemFailed(unit->mDc);
        ClearFFTUnitIfMemFailed(unit->mNyq);
        ClearFFTUnitIfMemFailed(unit->mPhase);
        ClearFFTUnitIfMemFailed(unit->mPhaseDiffs);
        unit->mNumBins = numbins;
        unit->mWritePtr = 0;
    } else if (numbins != unit->mNumBins) {
        // Cannot allow the FFT size to change
        return;
    }

    SCPolarBuf *p = ToPolarApx(buf);

    if (freezeState > 0.f) {
        RGET
        // Pull random DC and nyquist magnitudes
        p->dc = unit->mDc[rgen.irand(unit->mNumFrames)];
        p->nyq = unit->mNyq[rgen.irand(unit->mNumFrames)];
        for (int xxn = 0; xxn < unit->mNumBins; xxn++) {
            // For each bin, grab a random magnitude and phase diff pair
            int idx = rgen.irand(unit->mNumFrames);
            idx = idx * unit->mNumBins + xxn;
            p->bin[xxn].mag = unit->mMags[idx];
            unit->mPhase[xxn] = sc_wrap(unit->mPhase[xxn] + unit->mPhaseDiffs[idx], 0.f, static_cast<float>(twopi));
            p->bin[xxn].phase = unit->mPhase[xxn];
        }
    } else {
        // We're writing to a circular buffer, so pull the current magnitude and phase diff arrays
        float *currentMagArr = unit->mMags + (unit->mWritePtr * unit->mNumBins);
        float *currentPhaseDiffArr = unit->mPhaseDiffs + (unit->mWritePtr * unit->mNumBins);
        for (int xxn = 0; xxn < numbins; xxn++) {
            currentMagArr[xxn] = p->bin[xxn].mag;
            currentPhaseDiffArr[xxn] = sc_wrap(p->bin[xxn].phase - unit->mPhase[xxn], 0.f, static_cast<float>(twopi));
            unit->mPhase[xxn] = p->bin[xxn].phase;
        }
        unit->mDc[unit->mWritePtr] = p->dc;
        unit->mNyq[unit->mWritePtr] = p->nyq;
        unit->mWritePtr++;
        unit->mWritePtr %= unit->mNumFrames;
    }
}

static void PV_CFreeze_Ctor(PV_CFreeze *unit) {
    SETCALC(PV_CFreeze_next);
    OUT0(0) = IN0(0);
    unit->mMags = nullptr;
    unit->mDc = nullptr;
    unit->mNyq = nullptr;
    unit->mPhase = nullptr;
    unit->mPhaseDiffs = nullptr;
    int numFrames = IN0(2);
    // prevent the user from doing something nuts
    unit->mNumFrames = sc_wrap(numFrames, 1, 20);
}

static void PV_CFreeze_Dtor(PV_CFreeze *unit) {
    if (unit->mMags) {
        RTFree(unit->mWorld, unit->mMags);
        unit->mMags = nullptr;
    }
    if (unit->mDc) {
        RTFree(unit->mWorld, unit->mDc);
        unit->mDc = nullptr;
    }
    if (unit->mNyq) {
        RTFree(unit->mWorld, unit->mNyq);
        unit->mNyq = nullptr;
    }
    if (unit->mPhase) {
        RTFree(unit->mWorld, unit->mPhase);
        unit->mPhase = nullptr;
    }
    if (unit->mPhaseDiffs) {
        RTFree(unit->mWorld, unit->mPhaseDiffs);
        unit->mPhaseDiffs = nullptr;
    }
}

static void PV_MagSqueeze_next(PV_MagSqueeze *unit, int inNumSamples) {
    PV_GET_BUF
    float low = IN0(1);
    float high = IN0(2);
    SCPolarBuf *p = ToPolarApx(buf);
    float min = p->dc;
    float max = p->dc;
    if (p->nyq < min)
        min = p->nyq;
    if (p->nyq > max)
        max = p->nyq;
    for (int i = 0; i < numbins; i++) {
        if (p->bin[i].mag < min)
            min = p->bin[i].mag;
        if (p->bin[i].mag > max)
            max = p->bin[i].mag;
    }
    float range = high - low;
    p->dc = (p->dc / max) * range + low;
    p->nyq = (p->nyq / max) * range + low;
    for (int i = 0; i < numbins; i++) {
        p->bin[i].mag = (p->bin[i].mag / max) * range + low;
    }
}

static void PV_MagSqueeze_Ctor(PV_MagSqueeze *unit) {
    SETCALC(PV_MagSqueeze_next);
    OUT0(0) = IN0(0);
}

static void PV_MagMirror_next(PV_MagMirror *unit, int inNumSamples) {
    PV_GET_BUF
    SCPolarBuf *p = ToPolarApx(buf);
    float min = p->dc;
    float max = p->dc;
    if (p->nyq < min)
        min = p->nyq;
    if (p->nyq > max)
        max = p->nyq;
    for (int i = 0; i < numbins; i++) {
        if (p->bin[i].mag < min)
            min = p->bin[i].mag;
        if (p->bin[i].mag > max)
            max = p->bin[i].mag;
    }
    p->dc = max - p->dc + min;
    p->nyq = max - p->nyq + min;
    for (int i = 0; i < numbins; i++) {
        p->bin[i].mag = max - p->bin[i].mag + min;
    }
}

static void PV_MagMirror_Ctor(PV_MagMirror *unit) {
    SETCALC(PV_MagMirror_next);
    OUT0(0) = IN0(0);
}

static void PV_MagXFade_next(PV_MagXFade *unit, int inNumSamples) {
    PV_GET_BUF2
    float crossfade = IN0(2);
    crossfade = sc_clip(crossfade, 0.f, 1.f);
    SCPolarBuf *p = ToPolarApx(buf1);
    SCPolarBuf *q = ToPolarApx(buf2);
    // use sqrt crossfade (https://dsp.stackexchange.com/questions/37477/understanding-equal-power-crossfades)
    float pCoef = 1.f, qCoef = 0.f;
    if (crossfade == 1.f) {
        pCoef = 0.f;
        qCoef = 1.f;
    } else if (crossfade > 0.f) {
        pCoef = sc_sqrt(1.f - crossfade);
        qCoef = sc_sqrt(crossfade);
    }
    p->dc = p->dc * pCoef + q->dc * qCoef;
    p->nyq = p->nyq * pCoef + q->nyq * qCoef;
    for (int i = 0; i < numbins; i++) {
        p->bin[i].mag = p->bin[i].mag * pCoef + q->bin[i].mag * qCoef;
    }
}

static void PV_MagXFade_Ctor(PV_MagXFade *unit) {
    SETCALC(PV_MagXFade_next);
    OUT0(0) = IN0(0);
}

PluginLoad(PV_Jeff) {
    ft = inTable;
    DefineSimpleUnit(PV_MagMirror);
    DefineSimpleUnit(PV_MagSqueeze);
    DefineSimpleUnit(PV_MagXFade);
    DefineDtorUnit(PV_CFreeze);
}
