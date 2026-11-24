/*
File: utilities.hpp
Author: Jeff Martin

Description:
A collection of utilities for phase vocoder stuff

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

#include <cmath>
#include "SC_Constants.h"

/// Linear interpolation between a and b
/// 
/// \param a The first value
/// \param b The second value
/// \param pos The position between a and b (0 <= pos <= 1)
/// \return The interpolated value
inline double interp(double a, double b, double pos) {
    return a + (b-a) * pos;
}

/// Linear interpolation between a and b
/// 
/// \param a The first value
/// \param b The second value
/// \param pos The position between a and b (0 <= pos <= 1)
/// \return The interpolated value
inline float interp(float a, float b, float pos) {
    return a + (b-a) * pos;
}

/// Power interpolation between a and b
/// 
/// \param a The first value
/// \param b The second value
/// \param pos The position between a and b (0 <= pos <= 1)
/// \return The interpolated value
inline double interpPower(double a, double b, double pos) {
    return std::sqrt(a*a + (b*b-a*a) * pos);
}

/// Power interpolation between a and b
/// 
/// \param a The first value
/// \param b The second value
/// \param pos The position between a and b (0 <= pos <= 1)
/// \return The interpolated value
inline float interpPower(float a, float b, float pos) {
    return std::sqrt(a*a + (b*b-a*a) * pos);
}

/// Principal argument computation for the RTPGHI algorithm
///
/// \param theta The phase to compute the principal argument for
/// \return The principal argment between 0 and 2pi
inline double princargRtpghi(double theta) {
    return theta - twopi * std::round(theta / twopi);
}

/// Principal argument computation for the RTPGHI algorithm
///
/// \param theta The phase to compute the principal argument for
/// \return The principal argment between 0 and 2pi
inline float princargRtpghi(float theta) {
    return theta - twopi * std::round(theta / static_cast<float>(twopi));
}