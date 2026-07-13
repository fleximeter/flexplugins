/*
File: pv.cpp
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

#include "SC_InterfaceTable.h"
#include "pvCFreeze.hpp"
#include "pvOther.hpp"
#include "pvStretch.hpp"

InterfaceTable *ft;

PluginLoad(PV_flexplugins) {
    ft = inTable;
    registerUnit<FlexPlugins::PV_MagSqueeze>(ft, "PV_MagSqueeze", false);
    registerUnit<FlexPlugins::PV_MagMirror>(ft, "PV_MagMirror", false);
    registerUnit<FlexPlugins::PV_MagXFade>(ft, "PV_MagXFade", false);
    DefineDtorUnit(PV_PlayBufStretch);
    registerUnit<FlexPlugins::PV_CFreeze>(ft, "PV_CFreeze", false);
}
