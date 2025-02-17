#include "threadpool.hpp"
#include "actor.hpp"

ThreadPool::ThreadPool(size_t numThreads)
{
    for (size_t i = 0; i < numThreads; ++i) {
        workers.emplace_back([this]
        {
            while (true) {
                Actor *pendingActor;
                {
                    std::unique_lock<std::mutex> lock(this->queueMutex);
                    this->condition.wait(lock, [this]
                    {
                        return this->stop || !this->pendingActors.empty();
                    });

                    if(this->stop && this->pendingActors.empty()){
                        return;
                    }

                    pendingActor = this->pendingActors.front();
                    this->pendingActors.pop();
                }

                if(pendingActor->trySetProcessing()){
                    pendingActor->consumeMessages();
                    pendingActor->resetProcessing();
                }
            }
        });
    }
}

void ThreadPool::registerActor(Actor *actor)
{
    {
        std::unique_lock<std::mutex> lock(actorsMutex);
        actors[actor->getAddress()] = actor;
    }
    scheduleActor(actor);
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

void ThreadPool::scheduleActor(Actor *actor)
{
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        if (stop){
            throw std::runtime_error("enqueue on stopped ThreadPool");
        }
        pendingActors.push(actor);
    }
    condition.notify_one();
}
