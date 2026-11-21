/*
File: pqueue.hpp
Author: Jeff Martin

Description:
A min and max heap implemented using arrays.
Functions as a priority queue.

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
#include <utility>

namespace FlexPlugins {
    template <typename K, typename V>
    class TestMinPQueue;

    template <typename K, typename V>
    class TestMaxPQueue;

    template <typename K, typename V>
    class PQueue {
        public:
        PQueue();

        /// Clears the queue
        void clear();

        virtual bool insert(K key, V val) = 0;

        virtual std::pair<K, V> pop() = 0;

        /// Peeks at the top of the priority queue without removing it
        ///
        /// \returns The top element on the priority queue
        std::pair<K, V> peek() const;
        
        /// Gets the maximum number of items that can be stored in the priority queue
        ///
        /// \returns The maximum number of items that can be stored in the priority queue
        size_t maxSize() const;

        /// Loads memory into the priority queue
        ///
        /// \param mem The memory to load
        /// \param nbytes The number of bytes allocated
        void memLoad(void* mem, size_t nbytes);

        /// Retrieves the priority queue memory for destruction
        ///
        /// \returns The memory allocated for the priority queue
        void* memRetrieve();

        /// Gets the size required for memory allocation for the priority queue
        ///
        /// \param queueSize The intended size of the priority queue
        /// \returns The size required for the priority queue memory
        size_t memSize(size_t queueSize) const;

        /// Gets the number of items stored in the priority queue
        ///
        /// \returns The number of items stored in the priority queue
        size_t size() const;
        
    protected:
        std::pair<K, V>* m_heap;
        size_t m_size;
        size_t m_maxSize;
    };

    template <typename K, typename V>
    class MinPQueue : public PQueue<K, V> {
    public:
        /// Constructs the priority queue
        ///
        /// \param maxSize The max size allowed in the priority queue
        MinPQueue() : PQueue<K, V>() {}

        /// Inserts an element into the priority queue
        ///
        /// \param data The data to insert
        /// \returns Whether or not the insert was successful
        bool insert(K key, V val) override;

        /// Removes and returns the top element on the priority queue
        ///
        /// \returns The top element on the priority queue
        std::pair<K, V> pop() override;

        friend class TestMinPQueue<K, V>;
    };

    template <typename K, typename V>
    class MaxPQueue : public PQueue<K, V> {
    public:
        /// Constructs the priority queue
        ///
        /// \param maxSize The max size allowed in the priority queue
        MaxPQueue() : PQueue<K, V>() {}

        /// Inserts an element into the priority queue
        ///
        /// \param data The data to insert
        /// \returns Whether or not the insert was successful
        bool insert(K key, V val) override;

        /// Removes and returns the top element on the priority queue
        ///
        /// \returns The top element on the priority queue
        std::pair<K, V> pop() override;

        friend class TestMaxPQueue<K, V>;
    };
}

template <typename K, typename V>
FlexPlugins::PQueue<K, V>::PQueue() {
    m_maxSize = 0;
    m_heap = nullptr;
    m_size = 0;
}

template <typename K, typename V>
void FlexPlugins::PQueue<K, V>::clear() {
    m_size = 0;
}

template <typename K, typename V>
size_t FlexPlugins::PQueue<K, V>::maxSize() const {
    return m_maxSize - 1;
}

template <typename K, typename V>
size_t FlexPlugins::PQueue<K, V>::memSize(size_t queueSize) const {
    return (queueSize + 1) * sizeof(std::pair<K, V>);
}

template <typename K, typename V>
void FlexPlugins::PQueue<K, V>::memLoad(void* mem, size_t nbytes) {
    m_heap = static_cast<std::pair<K, V>*>(mem);
    m_maxSize = nbytes / sizeof(std::pair<K, V>);
}

template <typename K, typename V>
void* FlexPlugins::PQueue<K, V>::memRetrieve() {
    return static_cast<void*>(m_heap);
}

template <typename K, typename V>
std::pair<K, V> FlexPlugins::PQueue<K, V>::peek() const {
    if (m_size > 1) {
        return m_heap[1];
    } else {
        return 0;
    }
}

template <typename K, typename V>
size_t FlexPlugins::PQueue<K, V>::size() const {
    if (m_size > 0) {
        return m_size - 1;
    } else {
        return 0;
    }
}

template <typename K, typename V>
bool FlexPlugins::MinPQueue<K, V>::insert(K key, V val) {
    if (this->m_size == 0) {
        this->m_size++;
    }
    if (this->m_size == this->m_maxSize) {
        return false;
    } else {
        size_t idx = this->m_size;
        this->m_heap[idx] = std::pair<K, V>(key, val);
        this->m_size++;

        // Bubble up
        while (idx > 1) {
            size_t parentIdx = idx;
            if (parentIdx % 2 == 1)
                parentIdx--;
            parentIdx >>= 1;
            if (this->m_heap[idx].first < this->m_heap[parentIdx].first) {
                std::pair<K, V> swapVal = this->m_heap[idx];
                this->m_heap[idx] = this->m_heap[parentIdx];
                this->m_heap[parentIdx] = swapVal;
            }
            idx = parentIdx;
        }
        return true;
    }
}

template <typename K, typename V>
bool FlexPlugins::MaxPQueue<K, V>::insert(K key, V val) {
    if (this->m_size == 0) {
        this->m_size++;
    }
    if (this->m_size == this->m_maxSize) {
        return false;
    } else {
        size_t idx = this->m_size;
        this->m_heap[idx] = std::pair<K, V>(key, val);
        this->m_size++;

        // Bubble up
        while (idx > 1) {
            size_t parentIdx = idx;
            if (parentIdx % 2 == 1)
                parentIdx--;
            parentIdx >>= 1;
            if (this->m_heap[idx].first > this->m_heap[parentIdx].first) {
                std::pair<K, V> swapVal = this->m_heap[idx];
                this->m_heap[idx] = this->m_heap[parentIdx];
                this->m_heap[parentIdx] = swapVal;
            }
            idx = parentIdx;
        }
        return true;
    }
}

template <typename K, typename V>
std::pair<K, V> FlexPlugins::MinPQueue<K, V>::pop() {
    if (this->m_size > 1) {
        std::pair<K, V> val = this->m_heap[1];
        this->m_heap[1] = this->m_heap[this->m_size-1];
        this->m_size--;
        size_t idx = 1;
        // Bubble down
        while (idx < this->m_size) {
            size_t leftChild = idx * 2;
            size_t rightChild = leftChild + 1;
            if (rightChild < this->m_size) {
                if (this->m_heap[leftChild].first <= this->m_heap[rightChild].first && this->m_heap[idx].first > this->m_heap[leftChild].first) {
                    std::pair<K, V> swapVal = this->m_heap[leftChild];
                    this->m_heap[leftChild] = this->m_heap[idx];
                    this->m_heap[idx] = swapVal;
                    idx = leftChild;
                } else if (this->m_heap[leftChild].first > this->m_heap[rightChild].first && this->m_heap[idx].first > this->m_heap[rightChild].first) {
                    std::pair<K, V> swapVal = this->m_heap[rightChild];
                    this->m_heap[rightChild] = this->m_heap[idx];
                    this->m_heap[idx] = swapVal;
                    idx = rightChild;
                } else {
                    break;
                }
            } else if (leftChild < this->m_size) {
                if (this->m_heap[idx].first > this->m_heap[leftChild].first) {
                    std::pair<K, V> swapVal = this->m_heap[leftChild];
                    this->m_heap[leftChild] = this->m_heap[idx];
                    this->m_heap[idx] = swapVal;
                    idx = leftChild;
                }
                break;
            } else {
                break;
            }
        }
        return val;
    } else {
        std::pair<K, V> val;
        val.first = 0;
        return val;
    }
}

template <typename K, typename V>
std::pair<K, V> FlexPlugins::MaxPQueue<K, V>::pop() {
    if (this->m_size > 1) {
        std::pair<K, V> val = this->m_heap[1];
        this->m_heap[1] = this->m_heap[this->m_size-1];
        this->m_size--;
        size_t idx = 1;
        // Bubble down
        while (idx < this->m_size) {
            size_t leftChild = idx * 2;
            size_t rightChild = leftChild + 1;
            if (rightChild < this->m_size) {
                if (this->m_heap[leftChild].first >= this->m_heap[rightChild].first && this->m_heap[idx].first < this->m_heap[leftChild].first) {
                    std::pair<K, V> swapVal = this->m_heap[leftChild];
                    this->m_heap[leftChild] = this->m_heap[idx];
                    this->m_heap[idx] = swapVal;
                    idx = leftChild;
                } else if (this->m_heap[leftChild].first < this->m_heap[rightChild].first && this->m_heap[idx].first < this->m_heap[rightChild].first) {
                    std::pair<K, V> swapVal = this->m_heap[rightChild];
                    this->m_heap[rightChild] = this->m_heap[idx];
                    this->m_heap[idx] = swapVal;
                    idx = rightChild;
                } else {
                    break;
                }
            } else if (leftChild < this->m_size) {
                if (this->m_heap[idx].first < this->m_heap[leftChild].first) {
                    std::pair<K, V> swapVal = this->m_heap[leftChild];
                    this->m_heap[leftChild] = this->m_heap[idx];
                    this->m_heap[idx] = swapVal;
                    idx = leftChild;
                }
                break;
            } else {
                break;
            }
        }
        return val;
    } else {
        std::pair<K, V> val;
        val.first = 0;
        return val;
    }
}