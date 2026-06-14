/*
File: pvOther.cpp
Author: Jeff Martin

Description:
A collection of PV UGens for SuperCollider.

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

#include "pvOther.hpp"
#include "FFT_UGens.h"

extern InterfaceTable *ft;

void PV_MagSqueeze_next(PV_MagSqueeze *unit, int inNumSamples) {
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

void PV_MagSqueeze_Ctor(PV_MagSqueeze *unit) {
    SETCALC(PV_MagSqueeze_next);
    OUT0(0) = IN0(0);
}

void PV_MagMirror_next(PV_MagMirror *unit, int inNumSamples) {
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

void PV_MagMirror_Ctor(PV_MagMirror *unit) {
    SETCALC(PV_MagMirror_next);
    OUT0(0) = IN0(0);
}

void PV_MagXFade_next(PV_MagXFade *unit, int inNumSamples) {
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

void PV_MagXFade_Ctor(PV_MagXFade *unit) {
    SETCALC(PV_MagXFade_next);
    OUT0(0) = IN0(0);
}