#pragma once
#include "SC_Unit.h"
#include "rubberband/RubberBandStretcher.h"

struct RubberBandStretcher : public Unit {
    RubberBand::RubberBandStretcher* m_stretcher;
    size_t m_samplesToDiscard;
    float m_timeRatio;
    float m_pitchRatio;
    float m_formantRatio;
    int m_transientsMode;
    int m_detectorOption;
    int m_phaseOption;
    int m_pitchQuality;
};

void RubberBandStretcher_Ctor(RubberBandStretcher *unit);
void RubberBandStretcher_Dtor(RubberBandStretcher *unit);
void RubberBandStretcher_next(RubberBandStretcher *unit, int inNumSamples);