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
