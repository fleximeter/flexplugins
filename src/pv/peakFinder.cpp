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

FlexPlugins::Peak::Peak(size_t peak) : peak(peak) {}
FlexPlugins::Peak::Peak(size_t peak, size_t leftValley, size_t rightValley) : peak(peak), leftValley(leftValley), rightValley(rightValley) {}

FlexPlugins::PeakFinder::PeakFinder(size_t fftSize, size_t radius) {
    m_maxSize = fftSize/2-1;
    m_radius = radius;
    m_size = 0;
    peaks = nullptr;
    m_queueL = nullptr;
    m_queueR = nullptr;
}

void FlexPlugins::PeakFinder::memLoad(void* arr) {
    size_t *data = (size_t*)arr;
    m_queueL = data;
    m_queueR = data + m_radius;
    peaks = (Peak*)(data + 2 * m_radius);
}

size_t FlexPlugins::PeakFinder::memSize() const {
    return m_radius * 2 * sizeof(size_t) + m_maxSize * sizeof(Peak);
}

void* FlexPlugins::PeakFinder::memRetrieve() {
    return (void*)m_queueL;
}

void FlexPlugins::PeakFinder::clear() {
    m_size = 0;
}

size_t FlexPlugins::PeakFinder::maxSize() const {
    return m_maxSize;
}

size_t FlexPlugins::PeakFinder::size() const {
    return m_size;
}

void FlexPlugins::PeakFinder::analyze(const SCPolarBuf *buf) {
    // We can only perform the analysis if we have enough bins
    if (m_queueL && m_maxSize > m_radius * 2 + 1) {
        m_size = 0;  // clear any existing data
        
        size_t i = m_radius;
        while (i < m_maxSize - m_radius) {
            bool isMax = true;
            for (size_t j = i - m_radius; j < i; j++) {
                if (buf->bin[j].mag >= buf->bin[i].mag) {
                    isMax = false;
                    break;
                }
            }
            for (size_t j = i + 1; j <= i + m_radius; j++) {
                if (buf->bin[j].mag >= buf->bin[i].mag) {
                    isMax = false;
                    break;
                }
            }
            if (isMax) {
                peaks[m_size] = Peak(i);
                m_size++;
                i += m_radius + 1;
            } else {
                i++;
            }
        }

        // Find the left valley for the first peak, and the right valley
        // for the last peak.
        if (m_size > 0) {
            float min = buf->bin[0].mag;
            size_t argmin = 0;
            size_t k = 1;
            for (; k < peaks[0].peak; k++) {
                if (buf->bin[k].mag < min) {
                    min = buf->bin[k].mag;
                    argmin = k;
                }
            }
            peaks[0].leftValley = argmin;
            k = peaks[m_size-1].peak + 1;
            min = buf->bin[k].mag;
            argmin = k;
            for (k++; k < m_maxSize; k++) {
                if (buf->bin[k].mag < min) {
                    min = buf->bin[k].mag;
                    argmin = k;
                }
            }
            peaks[m_size-1].rightValley = argmin;
        } 
        
        // Find the remaining left and right valleys
        if (m_size > 1) {
            for (size_t j = 0; j < m_size-1; j++) {                
                size_t k = peaks[j].peak + 1;
                float min = buf->bin[k].mag;
                size_t argmin = k;
                for (k++; k < peaks[j+1].peak; k++) {
                    if (buf->bin[k].mag < min) {
                        min = buf->bin[k].mag;
                        argmin = k;
                    }
                }
                peaks[j].rightValley = argmin - 1;
                peaks[j+1].leftValley = argmin;
            }
        }
    }
}