/*
File: rubberBandStretcher.hpp
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

#pragma once
#include "SC_PlugIn.hpp"
#include "rubberband/RubberBandStretcher.h"

namespace FlexPlugins {
    class RubberBandStretcher : public SCUnit {
    public:
        RubberBandStretcher();
        ~RubberBandStretcher();

    private:
        void next(int inNumSamples);
        
        /// The stretcher
        RubberBand::RubberBandStretcher* m_stretcher;

        /// The number of initial output samples to discard
        size_t m_samplesToDiscard;

        // A collection of settings for the RubberBand stretcher
        float m_timeRatio;
        float m_pitchRatio;
        float m_formantRatio;
        int m_transientsMode;
        int m_detectorOption;
        int m_phaseOption;
        int m_pitchQuality;
    };
}
