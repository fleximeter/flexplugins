/*
File: hashtable.cpp
Author: Jeff Martin

Description:
A simple int hash set (table size must be power of 2)

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

#include "hashtable.hpp"
#include <stdint.h>
#include <cmath>

// hash function from claude
// table bits is the power of 2 size of the table
static inline uint32_t hash32(uint32_t key, int table_bits) {
    return (key * 2654435769u) >> (32 - table_bits);
}

FlexPlugins::IntHashSet::IntHashSet(double maxLoadFactor) :
    m_table(nullptr),
    m_rehashTable(nullptr),
    m_originalAlloc(nullptr),
    m_size(0),
    m_maxSize(0),
    m_tableBits(0),
    m_currentLoadFactor(0),
    m_hasTombstones(false)
{
    if (maxLoadFactor > 0 && maxLoadFactor < 1) {
        m_maxLoadFactor = maxLoadFactor;
    } else {
        m_maxLoadFactor = 0.7;
    }
}

void FlexPlugins::IntHashSet::clear() {
    m_size = 0;
    m_hasTombstones = false;
    m_currentLoadFactor = 0;
    for (size_t i = 0; i < m_maxSize; i++) {
        m_table[i].isTombstone = false;
        m_table[i].isNull = true;
        m_table[i].data = 0;
    }
}

size_t FlexPlugins::IntHashSet::maxSize() const {
    return m_maxSize;
}

size_t FlexPlugins::IntHashSet::size() const {
    return m_size;
}

void FlexPlugins::IntHashSet::memLoad(void* mem, size_t nbytes) {
    m_table = static_cast<IntHashSetNode*>(mem);
    m_originalAlloc = mem;
    m_maxSize = (nbytes / 2) / sizeof(IntHashSetNode);
    // hash table size must be power of 2
    m_tableBits = static_cast<size_t>(std::log2(static_cast<float>(m_maxSize)));
    m_maxSize = 0x1 << m_tableBits;
    // the upper half of the allocation will be the rehash table
    m_rehashTable = m_table + m_maxSize;
    for (size_t i = 0; i < m_maxSize; i++) {
        m_table[i] = IntHashSetNode();
    }
}

void* FlexPlugins::IntHashSet::memRetrieve() {
    return m_originalAlloc;
}

size_t FlexPlugins::IntHashSet::memSize(size_t maxSize) const {
    // need to allocate 2x size because we need second rehash table
    return maxSize * 2 * sizeof(IntHashSetNode);
}

bool FlexPlugins::IntHashSet::insert(int data) {
    if (m_currentLoadFactor > m_maxLoadFactor) {
        if (m_hasTombstones) {
            IntHashSetNode *swap = m_table;
            m_table = m_rehashTable;
            m_rehashTable = swap;
            m_hasTombstones = false;
            m_size = 0;
            m_currentLoadFactor = 0;
            for (size_t i = 0; i < m_maxSize; i++) {
                m_table[i].data = 0;
                m_table[i].isNull = true;
                m_table[i].isTombstone = false;
            }
            for (size_t i = 0; i < m_maxSize; i++) {
                if (!m_rehashTable[i].isNull) {
                    bool result = insert(m_rehashTable[i].data);
                    if (!result) return false;
                }
            }
        }
        if (m_currentLoadFactor > m_maxLoadFactor) {
            return false;
        }
    }

    // hash and insert the data
    size_t idx = hash32(static_cast<uint32_t>(data), m_tableBits);
    if (m_table[idx].isNull && !m_table[idx].isTombstone) {
        m_table[idx].data = data;
        m_table[idx].isNull = false;
        m_size++;
        m_currentLoadFactor += 1/static_cast<double>(m_maxSize);
        return true;
    } else if (m_table[idx].data == data && !m_table[idx].isTombstone) {
        // no duplicate inserts allowed
        return false;
    } else {
        // linear probing
        for (size_t i = idx+1; i < m_maxSize; i++) {
            if (m_table[i].isNull && !m_table[i].isTombstone) {
                m_table[i].data = data;
                m_table[i].isNull = false;
                m_size++;
                m_currentLoadFactor += 1/static_cast<double>(m_maxSize);
                return true;
            } else if (m_table[i].data == data && !m_table[i].isTombstone) {
                // no duplicate inserts allowed
                return false;
            }
        }
        for (size_t i = 0; i < idx; i++) {
            if (m_table[i].isNull && !m_table[i].isTombstone) {
                m_table[i].data = data;
                m_table[i].isNull = false;
                m_size++;
                m_currentLoadFactor += 1/static_cast<double>(m_maxSize);
                return true;
            } else if (m_table[i].data == data && !m_table[i].isTombstone) {
                // no duplicate inserts allowed
                return false;
            }
        }
        return false;
    }
}

int FlexPlugins::IntHashSet::ifind(int data) {
    int idx = static_cast<int>(hash32(static_cast<uint32_t>(data), m_tableBits));
    if (m_table[idx].isNull && !m_table[idx].isTombstone) {
        return -1;
    } else {
        if (!m_table[idx].isNull && m_table[idx].data == data) {
            return idx;
        } else {
            for (int i = idx+1; i < m_maxSize; i++) {
                // the probing would have stopped here, so the item was not inserted
                if (m_table[i].isNull) {
                    if (!m_table[i].isTombstone) {
                        return -1;
                    }
                } else if (m_table[i].data == data) {
                    return i;
                }
            }
            for (int i = 0; i < idx; i++) {
                // the probing would have stopped here, so the item was not inserted
                if (m_table[i].isNull) {
                    if (!m_table[i].isTombstone) {
                        return -1;
                    }
                } else if (m_table[i].data == data) {
                    return i;
                }
            }
        }
        return -1;
    }
}

bool FlexPlugins::IntHashSet::find(int data) {
    if (ifind(data) >= 0) {
        return true;
    } else {
        return false;
    }
}

bool FlexPlugins::IntHashSet::remove(int data) {
    int idx = ifind(data);
    if (idx < 0) {
        return false;
    } else {
        m_table[idx].data = 0;
        m_table[idx].isNull = true;
        m_table[idx].isTombstone = true;
        m_hasTombstones = true;
        m_size--;
        return true;
    }
}