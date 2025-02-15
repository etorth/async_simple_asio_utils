#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include <functional>
#include <vector>
#include <memory>
#include <cstdint>
#include <random>
#include <string>
#include <atomic>
#include <condition_variable>
#include <unordered_map>
#include <stdexcept>
#include <cstdio>
#include <cstdarg>
#include <ctime>

#include "message.hpp"
#include "printmessage.hpp"
#include "threadpool.hpp"
#include "actor.hpp"

constexpr int M = 20; // Number of actors
std::unique_ptr<ThreadPool> pool;
std::vector<std::unique_ptr<Actor>> actors;

int randReciever(int from)
{
    while(true){
        if(const int recv = std::rand() % static_cast<int>(actors.size()); recv != from){
            return recv;
        }
    }
    return 0;
}

void randSendMessage(int from)
{
    int messageCount = std::rand() % 5 + 1;
    for (int i = 0; i < messageCount; ++i) {
        int randomActorAddress = randReciever(from);
        std::string message("Hello from Actor!");

        if (std::rand() % 2 == 0) {
            actors[from]->send(randomActorAddress, 0, message, [from](const Message& reply)
            {
                printMessage("Actor %llu received reply for sequence number %d: %s\n", actors[from]->getAddress(), reply.respID, reply.content.c_str());
            });
        }
        else {
            actors[from]->send(randomActorAddress, 0, message);
        }
    }
}

int main()
{
    std::srand(std::time(nullptr));
    pool = std::make_unique<ThreadPool>(4);
    for (int i = 0; i < M; ++i) {
        actors.push_back(std::make_unique<Actor>(*pool, i));
        pool->registerActor(actors.back().get());
    }

    for(int i = 0; i < M; ++i){
        randSendMessage(i);
    }

    std::this_thread::sleep_for(std::chrono::seconds(1)); // Let the tasks finish

    actors.clear();
    pool.reset();

    printMessage("Gracefully exiting main\n");
    return 0;
}
