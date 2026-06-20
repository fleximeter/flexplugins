/*
File: peakFinder.cpp
Author: Jeff Martin

Description:
A peak finder for the Laroche/Dolson phase locking algorithm.

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

#include "peakFinder.hpp"

Peak::Peak(size_t peak) : peak(peak) {}
Peak::Peak(size_t peak, size_t leftValley, size_t rightValley) : peak(peak), leftValley(leftValley), rightValley(rightValley) {}

#define SHIFT(arr, idx, val) \
    for (size_t xxi = )

PeakFinder::PeakFinder(size_t fftSize, size_t radius) {
    m_maxSize = fftSize/2-1;
    m_radius = radius;
    m_size = 0;
    peaks = nullptr;
    m_queueL = nullptr;
    m_queueR = nullptr;
}

void PeakFinder::memLoad(void* arr) {
    size_t *data = (size_t*)arr;
    m_queueL = data;
    m_queueR = data + m_radius;
    peaks = (Peak*)(data + 2 * m_radius);
}

size_t PeakFinder::memSize() const {
    return m_radius * 2 * sizeof(size_t) + m_maxSize * sizeof(Peak);
}

void* PeakFinder::memRetrieve() {
    return (void*)m_queueL;
}

void PeakFinder::clear() {
    m_size = 0;
}

size_t PeakFinder::maxSize() const {
    return m_maxSize;
}

size_t PeakFinder::size() const {
    return m_size;
}

void PeakFinder::analyze(const SCPolarBuf *buf) {
    // We can only perform the analysis if we have enough bins
    if (m_queueL && m_maxSize > m_radius * 2 + 1) {
        m_size = 0;  // clear any existing data
        
        size_t xxi = m_radius;
        while (xxi < m_maxSize - m_radius) {
            bool isMax = true;
            for (size_t xxj = xxi - m_radius; xxj < xxi; xxj++) {
                if (buf->bin[xxj].mag >= buf->bin[xxi].mag) {
                    isMax = false;
                    break;
                }
            }
            for (size_t xxj = xxi + 1; xxj <= xxi + m_radius; xxj++) {
                if (buf->bin[xxj].mag >= buf->bin[xxi].mag) {
                    isMax = false;
                    break;
                }
            }
            if (isMax) {
                peaks[m_size] = Peak(xxi);
                m_size++;
                xxi += m_radius + 1;
            } else {
                xxi++;
            }
        }

        // Find the left valley for the first peak, and the right valley
        // for the last peak.
        if (m_size > 0) {
            float min = buf->bin[0].mag;
            size_t argmin = 0;
            size_t xxk = 1;
            for (; xxk < peaks[0].peak; xxk++) {
                if (buf->bin[xxk].mag < min) {
                    min = buf->bin[xxk].mag;
                    argmin = xxk;
                }
            }
            peaks[0].leftValley = argmin;
            xxk = peaks[m_size-1].peak + 1;
            min = buf->bin[xxk].mag;
            argmin = xxk;
            for (xxk++; xxk < m_maxSize; xxk++) {
                if (buf->bin[xxk].mag < min) {
                    min = buf->bin[xxk].mag;
                    argmin = xxk;
                }
            }
            peaks[m_size-1].rightValley = argmin;
        } 
        
        // Find the remaining left and right valleys
        if (m_size > 1) {
            for (size_t xxj = 0; xxj < m_size-1; xxj++) {                
                size_t xxk = peaks[xxj].peak + 1;
                float min = buf->bin[xxk].mag;
                size_t argmin = xxk;
                for (xxk++; xxk < peaks[xxj+1].peak; xxk++) {
                    if (buf->bin[xxk].mag < min) {
                        min = buf->bin[xxk].mag;
                        argmin = xxk;
                    }
                }
                peaks[xxj].rightValley = argmin - 1;
                peaks[xxj+1].leftValley = argmin;
            }
        }
    }
}
