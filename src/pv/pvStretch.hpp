/*
File: pvStretch.hpp
Author: Jeff Martin

Description:
A phase vocoder time stretcher.

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

struct PV_PlayBufStretch : public Unit {
    // The index of the buffer with STFT data
    float m_fbufnum;

    // The buffer with STFT data
    SndBuf *m_buf;

    // Between 0 and 1; represents the start position of playback.
    // If it jumps during playback, playback will be restarted
    // at the new m_startPos.
    float m_startPos;
    
    // A fractional STFT frame index. Unlike m_startPos, it corresponds to the 
    // integer index of the current STFT frame (from 0 to M-1).
    // Used for interpolating position.
    float m_pos;

    // For the first frame, we need to read phase data directly
    // instead of computing it.
    bool m_firstFrame;
};

void PV_PlayBufStretch_Ctor(PV_PlayBufStretch *unit);
void PV_PlayBufStretch_next(PV_PlayBufStretch *unit, int inNumSamples);
