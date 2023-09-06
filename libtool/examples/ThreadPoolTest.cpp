#include "libtool/ThreadPool.h"
#include <chrono>
#include <thread>
#include <iostream>

using namespace alpha;

int main() {
    ThreadPool pool(2);
    pool.set_task_queue_size(4);
    pool.start();
    for (int i = 0; i < 10; i++) {
        Task* task = new Task;
        task->func = [i, task] (void*) {
            std::this_thread::sleep_for(std::chrono::seconds(i));
            std::cout << "Task-" << i << ", ptr = " << task << ", tid=" << std::this_thread::get_id() << std::endl;
        };
        pool.schedule()->push(task);
    }
    std::this_thread::sleep_for(std::chrono::seconds(50));
    std::cout << "sleep end\n";
    return 0;
}