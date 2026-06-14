/*
File: arrayheap.hpp
Author: Jeff Martin

Description:
A simple array-based heap

Copyright © 2025 by Jeffrey Martin. All rights reserved.
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
#include <cstddef>

/// A min heap for ints.
typedef struct {
    int* heap;
    size_t size;
    size_t maxSize;
} IntMinHeap;

/// Inserts into the heap
int heapInsert(IntMinHeap* heap, int data);

/// Removes from the heap and returns the value popped. Returns 0 if the heap is empty.
int heapPop(IntMinHeap* heap);

/// Safe peek at the top of the heap
int heapPeek(IntMinHeap* heap);