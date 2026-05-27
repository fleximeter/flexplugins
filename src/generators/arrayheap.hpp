// A min heap for ints.
typedef struct {
    int* heap;
    size_t size;
    size_t maxSize;
} IntMinHeap;

// Inserts into the heap
int heapInsert(IntMinHeap* heap, int data) {
    if (heap->size == heap->maxSize) {
        return 0;
    } else {
        if (heap->size == 0) {
            heap->size++;
        }
        size_t idx = heap->size;
        heap->heap[idx] = data;
        heap->size++;

        // Bubble up
        while (idx > 1) {
            size_t parentIdx = idx;
            if (parentIdx % 2 == 1)
                parentIdx--;
            parentIdx >>= 1;
            if (heap->heap[idx] < heap->heap[parentIdx]) {
                int swapVal = heap->heap[idx];
                heap->heap[idx] = heap->heap[parentIdx];
                heap->heap[parentIdx] = swapVal;
            }
            idx = parentIdx;
        }
        return 1;
    }
}

// Removes from the heap and returns the value popped. Returns 0 if the heap is empty.
int heapPop(IntMinHeap* heap) {
    if (heap->size > 1) {
        int val = heap->heap[1];
        heap->heap[1] = heap->heap[heap->size-1];
        heap->size--;
        size_t idx = 1;
        // Bubble down
        while (idx < heap->size) {
            size_t leftChild = idx * 2;
            size_t rightChild = leftChild + 1;
            if (rightChild < heap->size) {
                if (heap->heap[leftChild] <= heap->heap[rightChild] && heap->heap[idx] > heap->heap[leftChild]) {
                    int swapVal = heap->heap[leftChild];
                    heap->heap[leftChild] = heap->heap[idx];
                    heap->heap[idx] = swapVal;
                    idx = leftChild;
                } else if (heap->heap[leftChild] > heap->heap[rightChild] && heap->heap[idx] > heap->heap[rightChild]) {
                    int swapVal = heap->heap[rightChild];
                    heap->heap[rightChild] = heap->heap[idx];
                    heap->heap[idx] = swapVal;
                    idx = rightChild;
                } else {
                    break;
                }
            } else if (leftChild < heap->size) {
                if (heap->heap[idx] > heap->heap[leftChild]) {
                    int swapVal = heap->heap[leftChild];
                    heap->heap[leftChild] = heap->heap[idx];
                    heap->heap[idx] = swapVal;
                    idx = leftChild;
                }
                break;
            } else {
                break;
            }
        }
        return val;
    } else {
        return 0;
    }
}

// Safe peek at the top of the heap
inline int heapPeek(IntMinHeap* heap) {
    if (heap->size > 1) {
        return heap->heap[1];
    } else {
        return -1;
    }
}
