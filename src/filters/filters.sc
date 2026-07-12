// File: filters.sc
// Author: Jeff Martin
//
// Description:
// A collection of SuperCollider filter UGens
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

FIR : UGen {
    *ar {
        arg coefs, in, mul=1, add=0;
        coefs = coefs.multichannelExpandRef(1);
        ^this.multiNewList(['audio', in, coefs]).madd(mul, add)
    }
    *kr {
        arg coefs, in, mul=1, add=0;
        coefs = coefs.multichannelExpandRef(1);
        ^this.multiNewList(['control', in, coefs]).madd(mul, add)
    }
    *new1 {
        arg rate, in, coefs;
        var derefCoefs;
        derefCoefs = coefs.dereference;
        derefCoefs = derefCoefs.flop.flat;
        ^super.new.rate_(rate).addToSynth.init([in] ++ derefCoefs)
    }
    init {
        arg theInputs;
        inputs = theInputs;
    }
}