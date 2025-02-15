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
        std::queue<Actor *> pendingActors;
        std::mutex queueMutex;
        std::condition_variable condition;
        bool stop = false;

        std::unordered_map<int, Actor *> actors;
        std::mutex actorsMutex;

    public:
        ThreadPool(size_t);
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
        void scheduleActor(Actor *);
};
