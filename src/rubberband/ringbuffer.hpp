// Workflow:
// 1. Make a RingBuffer.
// 2. Initialize the memory separately and provide the allocated buffer with initialize().
//    Make sure you choose a good buffer size--ideally it should be larger than either the expected
//    input or output read sizes.
// 3. You can call writeBlock() each time you have a new block of samples to write to the buffer.
// 4. Before reading, you should call isReadReady(). This tells you if a new block of samples
//    is available to read. Then you can call readBlock().

#pragma once
#include <cstddef>

template <typename T>
class TestRingBuffer;

template <typename T>
class RingBuffer {
public:
    RingBuffer<T>();
    /// Given an initialized buffer, associates it with the RingBuffer
    void initialize(T* buffer, size_t size);
    
    /// Determines if the buffer is ready to read `length` samples
    bool isReadReady(size_t length);

    /// Reads a block of specified length
    void readBlock(T* destination, size_t length);
    
    /// Retrieves the size
    size_t size();

    /// Writes a block of specified length
    void writeBlock(T* samples, size_t length);
    
    /// Zeros out the buffer
    void zero();
    T* m_buffer;
private:
    size_t m_size; // size of buffer overall
    size_t m_inputPointer; // input write pointer
    size_t m_outputPointer; // output read pointer
    int m_newSamples; // number of new samples accumulated since last read
    template <typename U>
    friend class TestRingBuffer;
};

template <typename T>
RingBuffer<T>::RingBuffer() {
    m_buffer = nullptr;
    m_size = 0;
    m_newSamples = 0;
    m_inputPointer = 0;
    m_outputPointer = 0;
}

template <typename T>
void RingBuffer<T>::initialize(T* buffer, size_t size) {
    m_buffer = buffer;
    m_size = size;
}

template <typename T>
void RingBuffer<T>::writeBlock(T* samples, size_t length) {
    if (m_buffer && m_size > 0) {
        size_t startPos = m_inputPointer;
        for (size_t i = 0; i < length; i++) {
            m_buffer[m_inputPointer] = samples[i];
            m_inputPointer++;
            if (m_inputPointer >= m_size) {
                m_inputPointer = 0;
            }
            m_newSamples++;
        }
    }
}

template <typename T>
void RingBuffer<T>::readBlock(T* destination, size_t length) {
    if (m_buffer && m_size > 0) {
        for (size_t i = 0; i < length; i++) {
            destination[i] = m_buffer[m_outputPointer];
            m_outputPointer++;
            if (m_outputPointer >= m_size) {
                m_outputPointer = 0;
            }
        }
        m_newSamples -= length;
        if (m_newSamples < 0) m_newSamples = 0;
    }
}

template <typename T>
bool RingBuffer<T>::isReadReady(size_t length) {
    if (m_newSamples >= length) {
        return true;
    } else {
        return false;
    }
}

template <typename T>
size_t RingBuffer<T>::size() {
    return m_size;
}

template <typename T>
void RingBuffer<T>::zero() {
    for (size_t i = 0; i < m_size; i++)
        m_buffer[i] = 0;
}