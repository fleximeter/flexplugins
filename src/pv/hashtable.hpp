/*
File: hashtable.hpp
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

#include <cstddef>

namespace FlexPlugins {
    class TestIntHashSet;
    
    struct IntHashSetNode {
        IntHashSetNode() : data(0), isTombstone(false), isNull(true) {}
        int data;
        bool isTombstone;
        bool isNull;
    };

    /// A key-only hash table with linear probing. No resizing is allowed--
    /// once the load factor is exceeded, all entries will fail until an
    /// item has been removed from the table. This is to avoid
    /// reallocation on the fly.
    class IntHashSet {
    public:
        /// Creates a new IntHashSet
        IntHashSet(double maxLoadFactor=0.7);

        /// Inserts an item into the hash set using linear probing
        ///
        /// \param data The data to insert
        /// \returns Whether or not insertion was successful
        bool insert(int data);

        /// Finds an item in the hash set
        ///
        /// \param data The data to find
        /// \returns Whether or not the data is present
        bool find(int data);

        /// Removes an item from the hash set
        ///
        /// \param data The data to remove
        /// \returns Whether or not the data was removed
        bool remove(int data);

        /// Clears the hash set
        void clear();
        
        /// Gets the maximum number of items that can be stored in the hash set
        ///
        /// \returns The maximum number of items that can be stored in the hash set
        size_t maxSize() const;

        /// Loads memory into the hash set
        ///
        /// \param mem The memory to load
        /// \param nbytes The number of bytes allocated
        void memLoad(void* mem, size_t nbytes);

        /// Retrieves the hash set memory for destruction
        ///
        /// \returns The memory allocated for the hash set
        void* memRetrieve();

        /// Gets the size required for memory allocation for the hash set
        ///
        /// \param maxSize The max size needed
        /// \returns The size required for the hash set memory
        size_t memSize(size_t maxSize) const;

        /// Gets the number of items stored in the hash set
        ///
        /// \returns The number of items stored in the hash set
        size_t size() const;

        double currentLoadFactor() const { return m_currentLoadFactor; }
        double maxLoadFactor() const { return m_maxLoadFactor; }
        size_t tableBits() const { return m_tableBits; }
        bool hasTombstones() const { return m_hasTombstones; }
        
    private:
        /// Finds the index of an item if it is stored in the hash set
        ///
        /// \param data The data to find
        /// \returns The index if it is present, or -1
        int ifind(int data);

        IntHashSetNode* m_table;
        IntHashSetNode* m_rehashTable;
        void* m_originalAlloc;
        size_t m_size;
        size_t m_maxSize;
        size_t m_tableBits;
        double m_currentLoadFactor;
        double m_maxLoadFactor;
        bool m_hasTombstones;

        friend class TestIntHashSet;
    };
}
