/*
File: peakFinder.hpp
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

#pragma once
#include "FFT_UGens.h"

class Peak {
public:
    Peak(size_t peak);
    Peak(size_t peak, size_t leftValley, size_t rightValley);
    size_t peak, leftValley, rightValley;
};

class PeakFinder {
public:
    /// Constructs the PeakFinder
    ///
    /// \param fftSize The FFT size
    PeakFinder(size_t fftSize, size_t radius);

    /// Finds peaks in the provided SCPolarBuf. Note that this buffer must correspond
    /// to the original FFT size provided for the PeakFinder--otherwise memory errors may occur.
    ///
    /// \param buf The buffer to analyze
    void analyze(const SCPolarBuf *buf);

    /// Loads memory from the external SuperCollider allocator. Memory is allocated
    /// using RTAlloc() and the size is specified by the memSize() method.
    ///
    /// \param arr The allocated memory
    void memLoad(void *arr);

    /// Gets the memory size required for the PeakFinder
    size_t memSize() const;

    /// Gets the max size of the PeakFinder
    ///
    /// \returns The max size
    size_t maxSize() const;

    /// Gets the current size of the PeakFinder (the number of peaks stored)
    ///
    /// \returns The current size
    size_t size() const;

    /// Clears the PeakFinder
    void clear();

    /// Gets a pointer to the memory that was allocated for the PeakFinder
    /// so that it can be deallocated
    ///
    /// \return The memory pointer
    void* memRetrieve();

    Peak *peaks;
private:
    size_t m_maxSize, m_size, m_radius;
    size_t *m_queueL, *m_queueR;
};
