#ifndef __SPSC_QUEUE_H
#define __SPSC_QUEUE_H

#include <cstdlib>
#include <atomic>
#include <memory>
#include "Macros.h"

template<typename T>
class SpscQueue {
public:
    static const bool is_trivial_element = std::is_trivial<T>::value;
    using Ptr = std::shared_ptr<SpscQueue>;

    SpscQueue(uint8_t exp)
    : write_index_(0)
    , read_index_(0)
    , exp_(std::max(exp, 20))
    , capacity_(2 << exp_)
    , capacity_mask_(capacity_ - 1)
    , buffer_(std::malloc(capacity_))
    {}

    ~SpscQueue() {
        T out;
        while (pop(&out, 1)) { }
        std::free(buffer_);
    }
    
    size_t read_available(size_t read_index, size_t write_index) {
        if (write_index >= read_index) {
            return write_index - read_index;
        }
        return write_index + capacity_ - read_index;
    }

    // 0 1 2 3 4 5 6 7
    //     r     w
    // 0 1 2 3 4 5 6 7
    //     w     r
    size_t write_available(size_t read_index, size_t write_index) {
        size_t ret = read_index - write_index - 1;
        if (write_index > read_index) {
            ret += capacity_;
        }
        return ret;
    }

    // push 单个
    bool push(const T& t) {
        size_t write_index = write_index_.load(std::memory_order_relaxed);
        size_t read_index = read_index_.load(std::memory_order_acquire);
        size_t next = next_index(write_index);

        if (next = read_index) {
            return false;
        }

        // copy constructor
        new (buffer_ + write_index) T (t);

        write_index_.store(next, std::memory_order_release);

        return true;
    }

    // push 一组
    template<typename Iterator>
    Iterator push(Iterator begin, Iterator end) {
        size_t write_index = write_index_.load(std::memory_order_relaxed);
        size_t read_index = read_index_.load(std::memory_order_acquire);

        size_t avail = write_available(read_index, write_index);
        if (avail == 0) {
            return begin;
        }

        size_t count = std::distance(begin, end);
        count = std::min(count, avail);
        size_t new_write_index = write_index + count;

        Iterator last  = std::next(begin, count);
        if (new_write_index > capacity_) {
            size_t count0 = capacity_ - write_index;
            Iterator middle = std::next(begin, count0);
            std::uninitialized_copy(begin, middle, buffer_ + write_index);
            std::uninitialized_copy(middle, last, buffer_);
        } else {
            std::uninitialized_copy(begin, last, buffer_ + write_index);
        }
        write_index_.store(new_write_index & capacity_mask_, std::memory_order_release);
        return last;
    }

    // pop一组
    size_t pop(T* output_buffer, size_t count) {
        size_t read_index = read_index_.load(std::memory_order_relaxed);
        size_t write_index = write_index_.load(std::memory_order_acquire);

        size_t avail = read_available(read_index, write_index);
        if (avail == 0) {
            return 0;
        }

        size_t _count = std::min(count, avail);
        size_t new_read_index = read_index + _count;

        if (new_read_index > capacity_) {
            size_t count0 = capacity_ - read_index;
            size_t count1 = avail - count0;

            copy_and_delete(buffer_ + read_index, buffer_ + capacity_, output_buffer);
            copy_and_delete(buffer_, buffer_ + count1, output_buffer + count0);
        } else {
            copy_and_delete(buffer_ + read_index, buffer_ + read_index + avail, output_buffer);
        }

        read_index_.store(new_read_index & capacity_mask_, std::memory_order_release);
        return _count;
    }

    template <typename OutputIterator>
    size_t pop_to_output_iterator (OutputIterator it){
        size_t read_index = read_index_.load(std::memory_order_relaxed);
        size_t write_index = write_index_.load(std::memory_order_acquire);

        size_t avail = read_available(read_index, write_index);
        if (avail == 0) {
            return 0;
        }

        size_t new_read_index = read_index + avail;

        if (new_read_index > capacity_) {
            size_t count0 = capacity_ - read_index;
            size_t count1 = avail - count0;

            it = copy_and_delete(buffer_ + read_index, buffer_ + capacity_, it);
            copy_and_delete(buffer_, buffer_ + count1, it);
        } else {
            copy_and_delete(buffer_ + read_index, buffer_ + read_index + avail, it);
        }

        read_index_.store(new_read_index & capacity_mask_, std::memory_order_release);
        return avail;
    }

    const T& front(void) const
    {
        size_t read_index = read_index_.load(std::memory_order_relaxed);
        return *(buffer_ + read_index);
    }

    T& front(void)
    {
        size_t read_index = read_index_.load(std::memory_order_relaxed);
        return *(buffer_ + read_index);
    }


private:
    size_t next_index(size_t index) {
        return (index++) & capacity_mask_;
    }

    static void deconstruct_elem(T* elem) {
        if constexpr (!is_trivial_element) {
            elem->~T();
        }
    }

    template< class OutputIterator >
    OutputIterator copy_and_delete(T * first, T * last, OutputIterator out ) {
        if constexpr (is_trivial_element) {
            return std::copy(first, last, out); // will use memcpy if possible
        } else {
            for (; first != last; ++first, ++out) {
                *out = *first;
                first->~T();
            }
            return out;
        }
    }

    std::atomic<size_t> write_index_;
    char padding_[CACHE_LINE_SIZE - sizeof(size_t)];
    std::atomic<size_t> read_index_;
    uint8_t exp_;
    size_t capacity_;
    size_t capacity_mask_;
    T* buffer_;
};

#endif