// File: PV_CFreeze.sc
// Author: Jeff Martin
//
// Description:
// This is a SuperColllider UGen based on the PV_MagFreeze and PV_Freeze classes.
// It produces a more flexible phase vocoder spectral freeze patterned after
// Jean-François Charles's Max implementation.
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

// PV_CFreeze is a variant of PV_Freeze, but maintains several buffers of
// input magnitudes in order to produce a less static frozen spectrum.
PV_CFreeze : PV_ChainUGen {
    *new {
        arg buffer, freeze = 0.0, frameMemory = 4;
        ^this.multiNew('control', buffer, freeze, frameMemory);
    }
}

// PV_ZeroRandomBins randomly zeroes the magnitudes of spectral bins
// given a probability. The same bins will be zeroed each time until
// the trigger is set again.
PV_BinRandomMask : PV_ChainUGen {
    *new {
        arg buffer, mask = 0.0, prob = 0.0, expCurve = -1.0, trigger = 0.0;
        ^this.multiNew('control', buffer, mask, prob, expCurve, trigger);
    }
}

// PV_MagMirror mirrors spectral magnitudes.
PV_MagMirror : PV_ChainUGen {
    *new {
        arg buffer;
        ^this.multiNew('control', buffer);
    }
}

// PV_MagSqueeze squeezes the magnitudes of all spectral bins to fit into the range [low, high].
PV_MagSqueeze : PV_ChainUGen {
    *new {
        arg buffer, low = 0.0, high = 1.0;
        ^this.multiNew('control', buffer, low, high);
    }
}

PV_MagSqueeze1 : PV_ChainUGen {
    *new {
        arg buffer;
        ^this.multiNew('control', buffer);
    }
}

// An equal power crossfade of the magnitudes of two FFT buffers
PV_MagXFade : PV_ChainUGen {
    *new {
        arg buffer1, buffer2, fade=0.0;
        ^this.multiNew('control', buffer1, buffer2, fade);
    }
}