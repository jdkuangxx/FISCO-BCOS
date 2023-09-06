#include "ThreadPool.h"

using namespace alpha;

void Thread::thread_func(Context contex) {

    auto run_task = [](Task* task) {
        task->run();
        delete task;
        task = nullptr;
    };

    while (!contex.schedule->stopping()) {
        Task* task = nullptr;
        // 1. 阻塞等待取任务
        contex.task_queue->pop_front_blocking(task);
        if (task) {
            run_task(task);
        } 

        // 2. 非阻塞取出所有任务，并执行
        while (contex.task_queue->pop_front_unblocking(task)) {
            run_task(task);
        }

        // 3. 处理完自己的任务后, 去其他线程的任务队列中去偷任务
        task = contex.schedule->steal_work(contex.index);
        if (task) {
            run_task(task);
        }
    }
}