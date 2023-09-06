#ifndef __MSG_QUEUE_H
#define __MSG_QUEUE_H

#include <functional>
#include <mutex>
#include <condition_variable>
#include "macros/Common.h"

namespace alpha
{

template<typename T>
class MsgQueue {
public:
    explicit MsgQueue(ssize_t cap = DEFAULT_CAPACITY) 
    : head_(0)
    , tail_(0)
    , capacity_(cap)
    , capacity_mask_(capacity_ - 1)
    , buffer_(nullptr) { }

    ~MsgQueue() {
        if (buffer_ == nullptr) {
            return;
        }
        T t;
        while(pop(t)) {
            if (default_handler_) {
                default_handler_(t);
            }
        }
        delete[] buffer_;
    }

    DISALLOW_COPY_AND_ASSIGN(MsgQueue);

    bool pop_front_unblocking(T& t) {
        std::unique_lock<std::mutex> head_lock(head_mtx_);
        {
            std::unique_lock<std::mutex> tail_lock(tail_mtx_);
            if (buffer_ == nullptr || read_available(head_, tail_) <= 0) {
                return false;
            }
        }
        ssize_t head = head_;
        head_ = prev(head);
        t = *(buffer_ + head);
        return true;
    }

    void pop_front_blocking(T& t) {
        std::unique_lock<std::mutex> head_lock(head_mtx_);
        {
            std::unique_lock<std::mutex> tail_lock(tail_mtx_);
            ssize_t avail = read_available(head_, tail_);
            while (buffer_ == nullptr || avail <= 0) {
                tail_lock.unlock();
                // NOTE: 这里要定时唤醒线程
                // 1. 唤醒后的线程需要先用pop_back_unblocking检测当前队列是否有任务，
                // 2. 没有之后再尝试去其他线程的队列中偷取任务(需要确认偷多少个运行，总不能一直偷别人的运行，而不运行自己的)
                // 3. 最后调用pop_front_blocking来消费当前队列的任务，没有任务就阻塞
                head_cond_.wait_for(head_lock, std::chrono::milliseconds(wait_time_));
                tail_lock.lock();
                avail = read_available(head_, tail_);
            }
        }
        ssize_t head = head_;
        head_ = prev(head);
        t = *(buffer_ + head);
    }

    bool pop_back_unblocking(T& t) {
        std::unique_lock<std::mutex> head_lock(head_mtx_);
        std::unique_lock<std::mutex> tail_lock(tail_mtx_);
        if (buffer_ == nullptr || read_available(head_, tail_) <= 0) {
            return false;
        }
        ssize_t tail = tail_;
        tail_ = next(tail);
        t = *(buffer_ + tail);
        return true;
    }

    void push(const T& t) {
        std::unique_lock<std::mutex> head_lock(head_mtx_);
        {
            std::unique_lock<std::mutex> tail_lock(tail_mtx_);
            if (buffer_ == nullptr || write_available(head_, tail_) <= 0) {
                ssize_t new_capacity = buffer_ == nullptr ?  capacity_ : next_capacity();
                expand(new_capacity);
            }
        }
        // lock head
        ssize_t head = head_;
        head_ = next(head);
        new (buffer_ + head) T(t);
        head_cond_.notify_one();
    }

private:
    void expand(ssize_t new_capacity) {
        T* buffer = new T[new_capacity];
        if (buffer_ != nullptr) {
            if (tail_ > head_) {
                std::uninitialized_copy(buffer_ + tail_, buffer_ + capacity_, buffer);
                std::uninitialized_copy(buffer_, buffer_ + head_, buffer + capacity_ - tail_);
            } else {
                std::uninitialized_copy(buffer_ + tail_, buffer_ + head_, buffer);
            }
        }
        capacity_ = new_capacity;
        capacity_mask_ = capacity_ - 1;
        head_ = read_available(head_, tail_);
        tail_ = 0;
        if (buffer_ != nullptr) {
            delete[] buffer_;
        }
        buffer_ = buffer;
    }

    ssize_t next_capacity() const {
        return capacity_ << 1;
    }

    ssize_t next(ssize_t index) const {
        return (++index) & capacity_mask_;
    }

    ssize_t prev(ssize_t index) const {
        return (index + capacity_ - 1) & capacity_mask_;
    }

    ssize_t write_available(ssize_t head, ssize_t tail) {
        return (tail + capacity_ - head - 1) & capacity_mask_;
    }

    ssize_t read_available(ssize_t head, ssize_t tail) {
        return (head + capacity_ - tail) & capacity_mask_;
    }

private:
    ssize_t head_;
    ssize_t tail_;
    ssize_t capacity_;
    ssize_t capacity_mask_;
    T* buffer_;
    std::mutex head_mtx_;
    std::mutex tail_mtx_;
    std::condition_variable head_cond_;
    ssize_t wait_time_ = DEFAULT_WAIT_TIME;
    std::function<void(const T& t)> default_handler_;
    static const int DEFAULT_WAIT_TIME = 500;
    static const int DEFAULT_CAPACITY = 1024;
};

} // namespace alpha


#endif