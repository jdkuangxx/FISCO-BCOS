#ifndef ___THREAD_POOL_H
#define ___THREAD_POOL_H

#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include "MsgQueue.h"

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
    TaskQueue* task_queue;
    Schedule* schedule;
    int index;
};

struct Thread {
    std::thread* thread;
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
        assert(threads_ == nullptr);
        assert(threads_->size() > 0);
    }

    ~Schedule() {threads_ = nullptr; }

    void push(Task* task) {
        int index = last_push_thread_index_.fetch_add(1, std::memory_order_acq_rel);
        auto thread = threads_->at(index % threads_->size());
        thread.task_queue->push(task);
    }

    template<typename Iterator>
    void push(Iterator begin, Iterator end) {
        auto task_nuum = std::distance(begin, end);
        int index = last_push_thread_index_.fetch_add(task_nuum, std::memory_order_acq_rel);
        for (auto iter = begin; iter != end; ++iter) {
            auto thread = threads_->at((index++) % threads_->size());
            thread.task_queue->push(task);
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
            if (index != cur_thread_index && thread.task_queue->pop_back_unblocking(task)) {
                last_steal_thread_index_.store(index, std::memory_order_release);
                break;
            }
            index = (index ++) % thread_size;
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
    // TODO: 初始化threads_, schedule_
    // 接口封装, 用于计算的单例线程池, parallel_for类接口的封装
private:
    ThreadQueue* threads_;
    Schedule* schedule_;
};    

} // namespace alpha


#endif