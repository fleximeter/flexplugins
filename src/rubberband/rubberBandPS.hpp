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