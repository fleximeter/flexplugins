/*
File: rubberband.cpp
Author: Jeff Martin

Description:
A high quality, formant-preserving live pitch shifter and time stretcher using the RubberBand library.

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

#include "SC_PlugIn.h"
#include "rubberBandPS.hpp"
#include "rubberBandStretcher.hpp"
#include "rubberBandStretcherBuf.hpp"

InterfaceTable *ft;

PluginLoad(RubberBandPlugins) {
    ft = inTable;
    DefineDtorUnit(RubberBandPS);
    DefineDtorUnit(RubberBandStretcher);
    DefineDtorUnit(RubberBandStretcherBuf);
}
