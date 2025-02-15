#pragma once
#include <vector>
#include <queue>
#include <thread>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include <memory>

class Actor;
class ThreadPool
{
    private:
        std::vector<std::thread> workers;
        std::queue<std::function<void()>> tasks;
        std::mutex queueMutex;
        std::condition_variable condition;
        bool stop = false;

        std::unordered_map<uint64_t, std::shared_ptr<Actor>> actors;
        std::mutex actorsMutex;

    public:
        ThreadPool(size_t numThreads)
        {
            for (size_t i = 0; i < numThreads; ++i) {
                workers.emplace_back([this]
                {
                    while (true) {
                        std::function<void()> task;
                        {
                            std::unique_lock<std::mutex> lock(this->queueMutex);
                            this->condition.wait(lock, [this]
                            {
                                return this->stop || !this->tasks.empty();
                            });

                            if(this->stop && this->tasks.empty()){
                                return;
                            }

                            task = std::move(this->tasks.front());
                            this->tasks.pop();
                        }
                        task();
                    }
                });
            }
        }

        ~ThreadPool()
        {
            {
                std::unique_lock<std::mutex> lock(queueMutex);
                stop = true;
            }
            condition.notify_all();
            for (std::thread &worker : workers){
                worker.join();
            }
        }

    public:
        void registerActor(std::shared_ptr<Actor> actor);
        std::shared_ptr<Actor> getActor(uint64_t address);
        void enqueue(std::function<void()> task);
        void scheduleActor(std::shared_ptr<Actor> actor);
};
