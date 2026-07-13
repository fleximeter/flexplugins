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

extern InterfaceTable *ft;

FlexPlugins::RubberBandPS::RubberBandPS() {
    float pitchRatio = in0(1);

    // Initialize the shifter
    // 0x01000000  // formant preserving
    // 0x00000000  // no formant preservation
    m_shifter = (RubberBand::RubberBandLiveShifter*)RTAlloc(mWorld, sizeof(RubberBand::RubberBandLiveShifter));
    new (m_shifter) RubberBand::RubberBandLiveShifter(static_cast<size_t>(sampleRate()), 1, 0x01000000);
    m_shifter->setPitchScale(sc_clip(pitchRatio, 1e-2, 64));
    m_blockSize = fullBufferSize();
    m_shifterBlockSize = m_shifter->getBlockSize();

    // Buffers for holding the samples to feed in and out of the shifter.
    // These buffers need to have the block size specified by the RubberBand shifter.
    m_shiftBufferIn = (float*)RTAlloc(mWorld, m_shifter->getBlockSize() * sizeof(float));
    m_shiftBufferOut = (float*)RTAlloc(mWorld, m_shifter->getBlockSize() * sizeof(float));

    // Make the ring buffers
    m_sendBuffer = (RingBuffer<float>*)RTAlloc(mWorld, sizeof(RingBuffer<float>));
    new (m_sendBuffer) RingBuffer<float>;
    m_sendBuffer->initialize(
        (float*)RTAlloc(
            mWorld, 
            (fullBufferSize() + m_shifter->getBlockSize()) * 3 * sizeof(float)), 
            (fullBufferSize() + m_shifter->getBlockSize()) * 3
        );
    m_receiveBuffer = (RingBuffer<float>*)RTAlloc(mWorld, sizeof(RingBuffer<float>));
    new (m_receiveBuffer) RingBuffer<float>;
    m_receiveBuffer->initialize(
        (float*)RTAlloc(
            mWorld,
            (fullBufferSize() + m_shifter->getBlockSize()) * 3 * sizeof(float)),
            (fullBufferSize() + m_shifter->getBlockSize()) * 3
        );

    // Initialize output ring buffer with zeros. If there's trouble, you might need to do this twice.
    for (size_t i = 0; i < m_shifter->getBlockSize(); i++) {
        m_shiftBufferIn[i] = 0.f;
    }

    set_calc_function<RubberBandPS, &RubberBandPS::next>();
    next(1);
}

FlexPlugins::RubberBandPS::~RubberBandPS() {
    if (m_sendBuffer) {
        if (m_sendBuffer->m_buffer) {
            RTFree(mWorld, m_sendBuffer->m_buffer);
        }
        RTFree(mWorld, m_sendBuffer);
    }
    if (m_receiveBuffer) {
        if (m_receiveBuffer->m_buffer) {
            RTFree(mWorld, m_receiveBuffer->m_buffer);
        }
        RTFree(mWorld, m_receiveBuffer);
    }
    if (m_shifter) {
        RTFree(mWorld, m_shifter);
    }
    if (m_shiftBufferIn) {
        RTFree(mWorld, m_shiftBufferIn);
    }
    if (m_shiftBufferOut) {
        RTFree(mWorld, m_shiftBufferOut);
    }
}

void FlexPlugins::RubberBandPS::next(int inNumSamples) {
    float pitchRatio = in0(1);
    float formantRatio = in0(2);
    const float* inBuf = in(0);
    float* outBuf = out(0);

    // Prepare the shifter, clipping the pitch and formant ratios for safety
    m_shifter->setPitchScale(sc_clip(pitchRatio, 1e-2, 64));
    if (formantRatio) {
        m_shifter->setFormantScale(sc_clip(formantRatio, 1e-2, 64));
    } else {
        m_shifter->setFormantScale(0.f);
    }

    // Feed the input into the shifter
    m_sendBuffer->writeBlock(inBuf, inNumSamples);

    if (m_sendBuffer->isReadReady(m_shifterBlockSize)) {
        m_sendBuffer->readBlock(m_shiftBufferIn, m_shifterBlockSize);
        m_shifter->shift(&m_shiftBufferIn, &m_shiftBufferOut);
        m_receiveBuffer->writeBlock(m_shiftBufferOut, m_shifterBlockSize);
    }
    
    // If there is a block of output samples ready, read it. (There should always be a block ready.)
    if (m_receiveBuffer->isReadReady(inNumSamples)) {
        m_receiveBuffer->readBlock(outBuf, inNumSamples);
    } else {
        // Zero out the output buffer
        for (size_t i = 0; i < inNumSamples; i++) {
            outBuf[i] = 0.f;
        }
    }
}