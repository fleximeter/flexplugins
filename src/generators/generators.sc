// File: generators.sc
// Author: Jeff Martin
//
// Description:
// A collection of SuperCollider generator UGens
//
// Copyright © 2025 by Jeffrey Martin. All rights reserved.
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

// ImpulseDropout is a version of Impulse that randomly drops a percentage of the impulses.
ImpulseDropout : UGen {
    *ar {
        arg freq = 440.0, phase = 0.0, dropFrac = 0.0, mul = 1.0, add = 0.0;
        ^this.multiNew('audio', freq, phase, dropFrac).madd(mul, add);
    }
    *kr {
        arg freq = 440.0, phase = 0.0, dropFrac = 0.0, mul = 1.0, add = 0.0;
        ^this.multiNew('control', freq, phase, dropFrac).madd(mul, add);
    }
    signalRange { ^\unipolar }
}

// ImpulseJitter is a version of Impulse that allows the addition of jitter to each impulse.
ImpulseJitter : UGen {
    *ar {
        arg freq = 440.0, phase = 0.0, jitterFrac = 0.0, mul = 1.0, add = 0.0;
        ^this.multiNew('audio', freq, phase, jitterFrac).madd(mul, add);
    }
    *kr {
        arg freq = 440.0, phase = 0.0, jitterFrac = 0.0, mul = 1.0, add = 0.0;
        ^this.multiNew('control', freq, phase, jitterFrac).madd(mul, add);
    }
    signalRange { ^\unipolar }
}

// LoopPhasor is a variant of Phasor with the following changes:
// 1. It has an embedded loop with start and end position (for playing samples with loop points).
//    This allows the LoopPhasor to be used for playing a Buffer normally, and then you only loop within a subset of the Buffer. 
// 2. There are two triggers. One is a trigger for returning to the start position. The other triggers an end to the looping behavior.
LoopPhasor : UGen {
    *ar { arg trigStart = 0.0, trigEnd = 0.0, rate = 1.0, start = 0.0, end = 1.0, loopStart = 0.0, loopEnd = 1.0;
        ^this.multiNew('audio', trigStart, trigEnd, rate, start, end, loopStart, loopEnd);
    }
    *kr { arg trigStart = 0.0, trigEnd = 0.0, rate = 1.0, start = 0.0, end = 1.0, loopStart = 0.0, loopEnd = 1.0;
        ^this.multiNew('control', trigStart, trigEnd, rate, start, end, loopStart, loopEnd);
    }
}
