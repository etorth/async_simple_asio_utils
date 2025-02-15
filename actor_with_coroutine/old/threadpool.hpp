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

        std::unordered_map<int, Actor *> actors;
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
        void registerActor(Actor *);
        Actor *getActor(int address);

    public:
        void enqueue(std::function<void()>);
        void scheduleActor(Actor *);
};
