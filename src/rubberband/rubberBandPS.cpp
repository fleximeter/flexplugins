/*
File: rubberBandPS.cpp
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

#include "rubberBandPS.hpp"
#include "SC_PlugIn.h"

extern InterfaceTable *ft;

void RubberBandPS_Ctor(RubberBandPS *unit) {
    float pitchRatio = IN0(1);

    // Initialize the shifter
    // 0x01000000  // formant preserving
    // 0x00000000  // no formant preservation
    unit->m_shifter = (RubberBand::RubberBandLiveShifter*)RTAlloc(unit->mWorld, sizeof(RubberBand::RubberBandLiveShifter));
    new (unit->m_shifter) RubberBand::RubberBandLiveShifter(static_cast<size_t>(SAMPLERATE), 1, 0x01000000);
    unit->m_shifter->setPitchScale(sc_clip(pitchRatio, 1e-2, 64));
    unit->m_blockSize = BUFLENGTH;
    unit->m_shifterBlockSize = unit->m_shifter->getBlockSize();

    // Buffers for holding the samples to feed in and out of the shifter.
    // These buffers need to have the block size specified by the RubberBand shifter.
    unit->m_shiftBufferIn = (float*)RTAlloc(unit->mWorld, unit->m_shifter->getBlockSize() * sizeof(float));
    unit->m_shiftBufferOut = (float*)RTAlloc(unit->mWorld, unit->m_shifter->getBlockSize() * sizeof(float));

    // Make the ring buffers
    unit->m_sendBuffer = (RingBuffer<float>*)RTAlloc(unit->mWorld, sizeof(RingBuffer<float>));
    new (unit->m_sendBuffer) RingBuffer<float>;
    unit->m_sendBuffer->initialize(
        (float*)RTAlloc(
            unit->mWorld, 
            (BUFLENGTH + unit->m_shifter->getBlockSize()) * 3 * sizeof(float)), 
            (BUFLENGTH + unit->m_shifter->getBlockSize()) * 3
        );
    unit->m_receiveBuffer = (RingBuffer<float>*)RTAlloc(unit->mWorld, sizeof(RingBuffer<float>));
    new (unit->m_receiveBuffer) RingBuffer<float>;
    unit->m_receiveBuffer->initialize(
        (float*)RTAlloc(
            unit->mWorld,
            (BUFLENGTH + unit->m_shifter->getBlockSize()) * 3 * sizeof(float)),
            (BUFLENGTH + unit->m_shifter->getBlockSize()) * 3
        );

    // Initialize output ring buffer with zeros. If there's trouble, you might need to do this twice.
    for (size_t i = 0; i < unit->m_shifter->getBlockSize(); i++) {
        unit->m_shiftBufferIn[i] = 0.f;
    }

    // Initialize first out sample
    OUT0(0) = 0;

    SETCALC(RubberBandPS_next);
}

void RubberBandPS_Dtor(RubberBandPS *unit) {
    RTFree(unit->mWorld, unit->m_sendBuffer->m_buffer);
    RTFree(unit->mWorld, unit->m_receiveBuffer->m_buffer);
    RTFree(unit->mWorld, unit->m_shifter);
    RTFree(unit->mWorld, unit->m_sendBuffer);
    RTFree(unit->mWorld, unit->m_receiveBuffer);
    RTFree(unit->mWorld, unit->m_shiftBufferIn);
    RTFree(unit->mWorld, unit->m_shiftBufferOut);
}

void RubberBandPS_next(RubberBandPS *unit, int inNumSamples) {
    float pitchRatio = IN0(1);
    float formantRatio = IN0(2);
    float* in = IN(0);
    float* out = OUT(0);

    // Prepare the shifter, clipping the pitch and formant ratios for safety
    unit->m_shifter->setPitchScale(sc_clip(pitchRatio, 1e-2, 64));
    if (formantRatio) {
        unit->m_shifter->setFormantScale(sc_clip(formantRatio, 1e-2, 64));
    } else {
        unit->m_shifter->setFormantScale(0.f);
    }

    // Feed the input into the shifter
    unit->m_sendBuffer->writeBlock(in, inNumSamples);

    if (unit->m_sendBuffer->isReadReady(unit->m_shifterBlockSize)) {
        unit->m_sendBuffer->readBlock(unit->m_shiftBufferIn, unit->m_shifterBlockSize);
        unit->m_shifter->shift(&(unit->m_shiftBufferIn), &(unit->m_shiftBufferOut));
        unit->m_receiveBuffer->writeBlock(unit->m_shiftBufferOut, unit->m_shifterBlockSize);
    }
    
    // If there is a block of output samples ready, read it. (There should always be a block ready.)
    if (unit->m_receiveBuffer->isReadReady(inNumSamples)) {
        unit->m_receiveBuffer->readBlock(out, inNumSamples);
    } else {
        // Zero out the output buffer
        for (size_t i = 0; i < inNumSamples; i++) {
            out[i] = 0.f;
        }
    }
}