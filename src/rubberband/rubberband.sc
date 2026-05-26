// File: rubberband.sc
// Author: Jeff Martin
//
// Description:
// A high quality, formant-preserving live pitch shifter and time stretcher using the RubberBand library.
// 
// Copyright © 2026 by Jeffrey Martin. All rights reserved.
// Website: https://www.jeffreymartincomposer.com
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

// RubberBandPS is a phase vocoder based pitch shifter using the Rubber Band 
// library. The difference between this and the RubberBandStretcher implementation
// is that it uses the RubberBandLiveShifter, which has a shorter start delay
// for pitch shifting.
RubberBandPS : UGen {
    *ar {
        arg in, pitchRatio=1.0, formantRatio=0.0, mul=1.0, add=0.0;
        ^this.multiNew('audio', in, pitchRatio, formantRatio).madd(mul, add);
    }
}

// RubberBandStretcher is a phase vocoder-based time stretcher/pitch shifter
// using the Rubber Band library.
// For obvious reasons, the timeRatio cannot be lower than 1.0 and will be
// clipped internally if necessary.
RubberBandStretcher : UGen {
    *ar {
        arg in, timeRatio=1.0, pitchRatio=1.0, formantRatio=0.0, transientsMode=0, 
            detectorMode=0, phaseMode=0, pitchQuality=0, windowOption=0, 
            smoothing=0, engine=0, mul=1.0, add=0.0;
        ^this.multiNew('audio', in, timeRatio, pitchRatio, formantRatio, transientsMode,
            detectorMode, phaseMode, pitchQuality, windowOption, smoothing, engine)
            .madd(1.0, 0.0);
    }
}
