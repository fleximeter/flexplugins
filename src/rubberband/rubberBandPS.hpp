/*
File: rubberBandPS.hpp
Author: Jeff Martin

Description:
A high-quality formant preserving pitch shifter using the RubberBand library.

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
#include "rubberband/RubberBandLiveShifter.h"
#include "ringbuffer.hpp"

struct RubberBandPS : public Unit {
    RubberBand::RubberBandLiveShifter* m_shifter;
    RingBuffer<float>* m_sendBuffer;
    RingBuffer<float>* m_receiveBuffer;
    float *m_shiftBufferIn;
    float *m_shiftBufferOut;
    size_t m_blockSize;
    size_t m_shifterBlockSize;
};

void RubberBandPS_Ctor(RubberBandPS *unit);
void RubberBandPS_Dtor(RubberBandPS *unit);
void RubberBandPS_next(RubberBandPS *unit, int inNumSamples);