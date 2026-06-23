/*
File: pvOther.hpp
Author: Jeff Martin

Description:
A collection of PV UGens for SuperCollider.

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
#include "SC_Unit.h"

struct PV_MagSqueeze : public Unit {};
void PV_MagSqueeze_next(PV_MagSqueeze *unit, int inNumSamples);
void PV_MagSqueeze_Ctor(PV_MagSqueeze *unit);

struct PV_MagMirror : public Unit {};
void PV_MagMirror_next(PV_MagMirror *unit, int inNumSamples);
void PV_MagMirror_Ctor(PV_MagMirror *unit);

struct PV_MagXFade : public Unit {};
void PV_MagXFade_next(PV_MagXFade *unit, int inNumSamples);
void PV_MagXFade_Ctor(PV_MagXFade *unit);