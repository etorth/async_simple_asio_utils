#include "threadpool.hpp"
#include "actor.hpp"

void ThreadPool::registerActor(std::shared_ptr<Actor> actor)
{
    std::unique_lock<std::mutex> lock(actorsMutex);
    actors[actor->getAddress()] = actor;
}

std::shared_ptr<Actor> ThreadPool::getActor(uint64_t address)
{
    std::unique_lock<std::mutex> lock(actorsMutex);
    return actors[address];
}

void ThreadPool::enqueue(std::function<void()> task)
{
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        if (stop) throw std::runtime_error("enqueue on stopped ThreadPool");
        tasks.push(std::move(task));
    }
    condition.notify_one();
}

void ThreadPool::scheduleActor(std::shared_ptr<Actor> actor)
{
    enqueue([actor, this]()
    {
        if (actor->trySetProcessing()){
            actor->consumeMessages();
            actor->resetProcessing();
        }
    });
}
