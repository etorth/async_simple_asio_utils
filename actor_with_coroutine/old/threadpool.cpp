#include "threadpool.hpp"
#include "actor.hpp"

void ThreadPool::registerActor(Actor *actor)
{
    std::unique_lock<std::mutex> lock(actorsMutex);
    actors[actor->getAddress()] = actor;
}

Actor *ThreadPool::getActor(int address)
{
    std::unique_lock<std::mutex> lock(actorsMutex);
    if(auto p = actors.find(address); p != actors.end()){
        return p->second;
    }
    else{
        return nullptr;
    }
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

void ThreadPool::scheduleActor(Actor *actor)
{
    enqueue([actor, this]()
    {
        if(actor->trySetProcessing()){
            actor->consumeMessages();
            actor->resetProcessing();
        }
    });
}
