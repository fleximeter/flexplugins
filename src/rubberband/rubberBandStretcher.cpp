/*
File: rubberBandStretcher.cpp
Author: Jeff Martin

Description:
A high-quality formant preserving pitch shifter and time stretcher using the RubberBand library.

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

#include "rubberBandStretcher.hpp"
#include <limits>

extern InterfaceTable *ft;

FlexPlugins::RubberBandStretcher::RubberBandStretcher() {
    float timeRatio = in0(1);
    float pitchRatio = in0(2);
    float formantRatio = in0(3);
    int transientsMode = static_cast<int>(in0(4));
    int detector = static_cast<int>(in0(5));
    int phaseOption = static_cast<int>(in0(6));
    int pitchQuality = static_cast<int>(in0(7));
    int windowOption = static_cast<int>(in0(8));
    int smoothing = static_cast<int>(in0(9));
    int engine = static_cast<int>(in0(10));
    
    m_timeRatio = timeRatio;
    m_pitchRatio = pitchRatio;
    m_formantRatio = formantRatio;
    m_transientsMode = transientsMode;
    m_detectorOption = detector;
    m_phaseOption = phaseOption;
    m_pitchQuality = pitchQuality;

    // Set up RubberBandStretcher initial options
    int options = 0x01000001;  // formant-preserving, real-time options set
    switch (transientsMode) {
        case 1:
            options |= 0x00000100;
            break;
        case 2:
            options |= 0x00000200;
            break;
    }
    switch (detector) {
        case 1:
            options |= 0x00000400;
            break;
        case 2:
            options |= 0x00000800;
            break;
    }
    switch (phaseOption) {
        case 1:
            options |= 0x00002000;
    }
    switch (pitchQuality) {
        case 1:
            options |= 0x02000000;
            break;
        case 2:
            options |= 0x04000000;
            break;
    }
    switch (windowOption) {
        case 1:
            options |= 0x00100000;
            break;
        case 2:
            options |= 0x00200000;
            break;
    }
    switch (smoothing) {
        case 1:
            options |= 0x00800000;
            break;
    }
    switch (engine) {
        case 1:
            options |= 0x20000000;
            break;
    }

    // Allocate the shifter with the given options
    m_stretcher = (RubberBand::RubberBandStretcher*)RTAlloc(mWorld, sizeof(RubberBand::RubberBandStretcher));
    new (m_stretcher) RubberBand::RubberBandStretcher(static_cast<size_t>(sampleRate()), 1, options, timeRatio, pitchRatio);

    // Initialize the shifter
    // The shifter accepts a block size (which must be set before the first process()
    // call and not after), which avoids the need to use local RingBuffers.
    m_stretcher->setMaxProcessSize(fullBufferSize());
    m_stretcher->setTimeRatio(sc_clip(timeRatio, 1.f, std::numeric_limits<float>::infinity()));
    m_stretcher->setPitchScale(sc_clip(pitchRatio, 1e-2, 64));
    m_stretcher->setFormantScale(sc_clip(formantRatio, 1e-2, 64));

    // Feed samples in until the shifter is ready to start producing valid output.
    // This is necessary because the shifter isn't ready to produce valid output
    // as soon as it is initialized--it requires padded 0s to be fed in for some
    // number of samples specified by the shifter.
    float *zeroBuf = (float*)RTAlloc(mWorld, fullBufferSize() * sizeof(float));
    for (size_t i = 0; i < fullBufferSize(); i++) {
        zeroBuf[i] = 0.f;
    }

    // The number of initial zeros required
    size_t startPad = m_stretcher->getPreferredStartPad();
    
    // The number of samples to discard at the beginning of the stretcher output.
    // This is handled in the RubberBandStretcher_next() method.
    m_samplesToDiscard = m_stretcher->getStartDelay();

    // Feed in the start pad samples
    while (startPad > 0) {
        m_stretcher->process(&zeroBuf, fullBufferSize(), false);
        startPad -= fullBufferSize();
    }
    RTFree(mWorld, zeroBuf);

    set_calc_function<RubberBandStretcher, &RubberBandStretcher::next>();
    next(1);
}

FlexPlugins::RubberBandStretcher::~RubberBandStretcher() {
    if (m_stretcher) RTFree(mWorld, m_stretcher);
}

void FlexPlugins::RubberBandStretcher::next(int inNumSamples) {
    const float *inBuf = in(0);
    float *outBuf = out(0);
    float timeRatio = in0(1);
    float pitchRatio = in0(2);
    float formantRatio = in0(3);
    int transientsMode = static_cast<int>(in0(4));
    int detector = static_cast<int>(in0(5));
    int phaseOption = static_cast<int>(in0(6));
    int pitchQuality = static_cast<int>(in0(7));

    // Update shifter options only if something has changed
    if (timeRatio != m_timeRatio) {
        m_stretcher->setTimeRatio(sc_clip(timeRatio, 1.f, std::numeric_limits<float>::infinity()));
    }
    if (pitchRatio != m_pitchRatio) {
        m_stretcher->setPitchScale(sc_clip(pitchRatio, 1e-2, 64));
    }
    if (formantRatio != m_formantRatio) {
        m_stretcher->setFormantScale(sc_clip(formantRatio, 1e-2, 64));
    }
    // QUESTION: Will this method of setting options override all existing options,
    // or just the option provided? May need to compute all options from scratch.
    if (transientsMode != m_transientsMode) {
        switch (transientsMode) {
            case 1:
                m_stretcher->setTransientsOption(0x00000100);
                break;
            case 2:
                m_stretcher->setTransientsOption(0x00000200);
                break;
            default:
                m_stretcher->setTransientsOption(0x00000000);
        }
    }
    if (detector != m_detectorOption) {
        switch (detector) {
            case 1:
                m_stretcher->setDetectorOption(0x00000400);
                break;
            case 2:
                m_stretcher->setDetectorOption(0x00000800);
                break;
            default:
                m_stretcher->setDetectorOption(0x00000000);
        }
    }
    if (phaseOption != m_phaseOption) {
        switch (phaseOption) {
            case 1:
                m_stretcher->setPhaseOption(0x00002000);
                break;
            default:
                m_stretcher->setPhaseOption(0x00000000);
        }
    }
    if (pitchQuality != m_pitchQuality) {
        switch (pitchQuality) {
            case 1:
                m_stretcher->setPitchOption(0x02000000);
                break;
            case 2:
                m_stretcher->setPitchOption(0x04000000);
                break;
            default:
                m_stretcher->setPitchOption(0x00000000);
        }
    }

    m_stretcher->process(&inBuf, inNumSamples, false);
    
    // If we can retrieve a full block worth of audio
    if (m_stretcher->available() >= inNumSamples) {
        m_stretcher->retrieve(&outBuf, inNumSamples);
        // Clear initial samples if necessary
        if (m_samplesToDiscard > 0) {
            size_t i = 0;
            while (i < inNumSamples && m_samplesToDiscard > 0) {
                outBuf[i] = 0.f;
                m_samplesToDiscard--;
                i++;
            }
        }
    }
    
    // Output zeros if the shifter has no new samples available
    else {
        ClearUnitOutputs(this, inNumSamples);
    }
}
