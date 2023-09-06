#ifndef ___THREAD_POOL_H
#define ___THREAD_POOL_H

#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <cassert>
#include "MsgQueue.h"
#include <iostream>

namespace alpha
{

struct Task{
    void* user_data = nullptr;
    std::function<void(void* user_data)> func;
    void run() {
        if (func) {
            func(user_data);
        }
    }
};

using TaskQueue = MsgQueue<Task*>;
class Schedule;

struct Context {
    TaskQueue* task_queue = nullptr;
    Schedule* schedule = nullptr;
    int index = 0;
};

struct Thread {
    std::thread* thread = nullptr;
    Context context;
    static void thread_func(Context contex);
};

using ThreadQueue = std::vector<Thread>;

class Schedule {
public:
    Schedule(ThreadQueue* threads) 
    : threads_(threads)
    , last_push_thread_index_(0)
    , last_steal_thread_index_(0)
    , stopping_(false) {
        assert(threads_ != nullptr);
        assert(threads_->size() > 0);
    }

    ~Schedule() { threads_ = nullptr; }

    void push(Task* task) {
        int index = last_push_thread_index_.fetch_add(1, std::memory_order_acq_rel);
        // std::cout << "push task-" << task << " to thread-" << (index % threads_->size()) << std::endl;
        auto thread = threads_->at(index % threads_->size());
        thread.context.task_queue->push(task);
    }

    template<typename Iterator>
    void push(Iterator begin, Iterator end) {
        auto task_nuum = std::distance(begin, end);
        int index = last_push_thread_index_.fetch_add(task_nuum, std::memory_order_acq_rel);
        for (auto iter = begin; iter != end; ++iter) {
            auto thread = threads_->at((index++) % threads_->size());
            thread.context.task_queue->push(*iter);
        }
    }

    void push(const std::vector<Task*>& tasks) {
        push(tasks.begin(), tasks.end());
    }

    Task* steal_work(int cur_thread_index) {
        Task* task = nullptr;
        size_t thread_size = threads_->size();
        size_t index = (last_steal_thread_index_.load(std::memory_order_acquire) + 1) % thread_size;
        size_t index_temp = index;
        do
        {
            auto thread = threads_->at(index);
            if (index != cur_thread_index && thread.context.task_queue->pop_back_unblocking(task)) {
                last_steal_thread_index_.store(index, std::memory_order_release);
                break;
            }
            index = (index + 1) % thread_size;
        } while (index != index_temp);
        return task;
    }

    bool stopping() const {
        return stopping_.load(std::memory_order_acquire);
    }

    void stop() {
        stopping_.store(true, std::memory_order_release);
    }

private:
    ThreadQueue* threads_;
    // NOTE: 入队的负载均衡策略 轮询
    std::atomic<int> last_push_thread_index_;
    // NOTE: 偷任务的负载均衡策略 轮询
    std::atomic<int> last_steal_thread_index_;
    std::atomic<bool> stopping_;
};

class ThreadPool {
public:
    explicit ThreadPool(int num_thread = std::thread::hardware_concurrency())
    : num_thread_(num_thread)
    , threads_(nullptr)
    , schedule_(nullptr) {}

    ~ThreadPool() {
        stop();
        clear();
    }

    void start() {
        threads_ = new ThreadQueue(num_thread_);
        schedule_ = new Schedule(threads_);
        for (size_t i = 0; i < threads_->size(); ++i) {
            auto& thread = threads_->at(i);
            thread.context.schedule = schedule_;
            thread.context.task_queue = new TaskQueue(task_queue_size_);
            thread.context.task_queue->set_wait_time(wait_time_);
            thread.context.index = i;
            thread.context.task_queue->set_clear_handler([](const Task* task) {
                std::cout << "delete not runing task-" << task << ", tid=" << std::this_thread::get_id() << std::endl;
                delete task;
            });
        }

        for (size_t i = 0; i < threads_->size(); ++i) {
            auto& thread = threads_->at(i);
            thread.thread = new std::thread([thread](){
                // std::cout << "index-" << thread.context.index << ", tid=" << std::this_thread::get_id() << std::endl;
                Thread::thread_func(thread.context);
            });
        }
    }

    void stop() {
        if (schedule_->stopping()) {
            return;
        }
        
        schedule_->stop();
        for (auto& th : *threads_) {
            th.context.task_queue->stop();
            if (th.thread->joinable()) {
                th.thread->join();
            }
        }
    }

    void clear() {
        if (threads_!= nullptr) {
            for (auto& th : *threads_) {
                delete th.thread;
                delete th.context.task_queue;
            }
            delete threads_;    
            threads_ = nullptr;
        }

        if (schedule_ != nullptr) {
            delete schedule_;
            schedule_ = nullptr;
        }
    }

    // 必须是2的N次方
    void set_task_queue_size(int size) { task_queue_size_ = size; }

    void set_wait_time(int time) { wait_time_ = time; }

    Schedule* schedule() { return schedule_; }
private:
    ThreadQueue* threads_;
    Schedule* schedule_;
    int num_thread_;
    int task_queue_size_ = DEFAULT_TASK_QUEUE_SIZE;
    int wait_time_ = DEFAULT_WAIT_TIME;
    static const int DEFAULT_WAIT_TIME = 100;
    static const int DEFAULT_TASK_QUEUE_SIZE = 1024;
};    

} // namespace alpha


#endif