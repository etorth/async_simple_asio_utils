#include "task.hpp"
#include "threadpool.hpp"
#include "sync_wait.hpp"

#include <iostream>
#include <thread>

task run_async_print(threadpool& pool)
{
    co_await pool.schedule();
    std::cout << "This is a hello from thread: " << std::this_thread::get_id() << "\n";
}

int main()
{
    std::cout << "The main thread id is: " << std::this_thread::get_id() << "\n";
    threadpool pool{8};
    task t = run_async_print(pool);
    sync_wait(t);
}
