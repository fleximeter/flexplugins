/*
File: rubberBandStretcherBuf.cpp
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

#include "rubberBandStretcherBuf.hpp"
#include <iostream>

extern InterfaceTable *ft;

FlexPlugins::RubberBandStretcherBuf::RubberBandStretcherBuf() {
    float timeRatio = in0(8);
    float pitchRatio = in0(9);
    float formantRatio = in0(10);
    int transientsMode = static_cast<int>(in0(11));
    int detector = static_cast<int>(in0(12));
    int phaseOption = static_cast<int>(in0(13));
    int pitchQuality = static_cast<int>(in0(14));
    int windowOption = static_cast<int>(in0(15));
    int smoothing = static_cast<int>(in0(16));
    int engine = static_cast<int>(in0(17));
    
    m_timeRatio = timeRatio;
    m_pitchRatio = pitchRatio;
    m_formantRatio = formantRatio;
    m_transientsMode = transientsMode;
    m_detectorOption = detector;
    m_phaseOption = phaseOption;
    m_pitchQuality = pitchQuality;

    // Acquire the sound buffer
    float fbufnum = in0(1);
    uint32 bufnum = static_cast<uint32>(fbufnum);
    if (bufnum >= mWorld->mNumSndBufs) bufnum = 0;
    m_fbufnum = fbufnum;
    m_buf = mWorld->mSndBufs + bufnum;
    m_writePtr = static_cast<size_t>(in0(2));  // initial offset
    m_prevTrigger = 0.f;

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
    m_stretcher->setTimeRatio(sc_clip(timeRatio, 1e-5, 1e5));
    m_stretcher->setPitchScale(sc_clip(pitchRatio, 1e-2, 64));
    m_stretcher->setFormantScale(sc_clip(formantRatio, 1e-2, 64));

    // Feed samples in until the shifter is ready to start producing valid output.
    // This is necessary because the shifter isn't ready to produce valid output
    // as soon as it is initialized--it requires padded 0s to be fed in for some
    // number of samples specified by the shifter.
    m_localBuf = (float*)RTAlloc(mWorld, fullBufferSize() * sizeof(float));
    for (size_t i = 0; i < fullBufferSize(); i++) {
        m_localBuf[i] = 0.f;
    }

    // The number of initial zeros required
    size_t startPad = m_stretcher->getPreferredStartPad();
    
    // The number of samples to discard at the beginning of the stretcher output.
    // This is handled in the RubberBandStretcher_next() method.
    m_samplesToDiscard = m_stretcher->getStartDelay();

    // Feed in the start pad samples
    while (startPad > 0) {
        m_stretcher->process(&m_localBuf, fullBufferSize(), false);
        startPad -= fullBufferSize();
    }

    // Initialize first out sample
    set_calc_function<RubberBandStretcherBuf, &RubberBandStretcherBuf::next>();
    next(1);
}

FlexPlugins::RubberBandStretcherBuf::~RubberBandStretcherBuf() {
    if (m_stretcher) RTFree(mWorld, m_stretcher);
    if (m_localBuf) RTFree(mWorld, m_localBuf);
}

void FlexPlugins::RubberBandStretcherBuf::next(int inNumSamples) {
    // Step 1: acquire the sound buffer
    const SndBuf *writeBuf = m_buf;
    if (!writeBuf) {
        std::cout << "WARNING: The stftBuffer could not be accessed. Aborting.\n";
        ClearUnitOutputs(this, inNumSamples);
        return;
    }
    ACQUIRE_SNDBUF_SHARED(writeBuf);
    float* bufData __attribute__((__unused__)) = writeBuf->data;
    const uint32 bufChannels __attribute__((__unused__)) = writeBuf->channels;
    const uint32 bufSamples __attribute__((__unused__)) = writeBuf->samples;
    const uint32 bufFrames = writeBuf->frames;
    
    if (bufChannels != 1) {
        std::cout << "WARNING: The buffer has " << bufChannels << " channels, but the " << 
            "RubberBandStretcherBuf only supports mono buffers. Aborting.\n";
        ClearUnitOutputs(this, inNumSamples);
        RELEASE_SNDBUF_SHARED(writeBuf);
        return;
    }

    // 2. Update unit parameters if required
    float timeRatio = in0(8);
    float pitchRatio = in0(9);
    float formantRatio = in0(10);
    int transientsMode = static_cast<int>(in0(11));
    int detector = static_cast<int>(in0(12));
    int phaseOption = static_cast<int>(in0(13));
    int pitchQuality = static_cast<int>(in0(14));

    // Update shifter options only if something has changed
    if (timeRatio != m_timeRatio) {
        m_stretcher->setTimeRatio(sc_clip(timeRatio, 1e-5, 1e5));
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

    // 3. Handle the trigger functionality
    float trigger = in0(7);
    if (trigger > 0.f && m_prevTrigger <= 0.f) {
        m_writePtr = 0;
    }
    m_prevTrigger = trigger;

    // 4. Process input audio.
    const float *inBuf = in(0);
    float recLevel = in0(3);
    float preLevel = in0(4);
    float loop = in0(6);
    
    m_stretcher->process(&inBuf, inNumSamples, false);

    while (m_stretcher->available() > 0) {
        size_t numRetrieve = static_cast<size_t>(std::min(inNumSamples, m_stretcher->available()));
        m_stretcher->retrieve(&m_localBuf, numRetrieve);
        // 5. While we run the audio through the stretcher regardless, we only store it to the buffer if "run" is set.
        if (in0(5) > 0.f) {
            for (size_t i = 0; i < numRetrieve; i++) {
                if (m_writePtr >= bufSamples) {
                    if (loop > 0.f) {
                        m_writePtr = 0;
                    } else {
                        RELEASE_SNDBUF_SHARED(writeBuf);
                        ClearUnitOutputs(this, inNumSamples);
                        DoneAction(static_cast<int>(in0(18)), this);
                        return;
                    }
                }

                // We need to ignore the initial samples output by the stretcher
                if (m_samplesToDiscard > 0) {
                    m_samplesToDiscard--;
                } else {
                    bufData[m_writePtr] = preLevel * bufData[m_writePtr] + recLevel * m_localBuf[i];
                }

                m_writePtr++;
            }
        }
    }

    ClearUnitOutputs(this, inNumSamples);
    RELEASE_SNDBUF_SHARED(writeBuf);
}