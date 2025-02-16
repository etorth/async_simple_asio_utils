#include <ctime>
#include <memory>
#include <vector>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <functional>

#include "message.hpp"
#include "printmessage.hpp"
#include "threadpool.hpp"
#include "actor.hpp"
#include "sync_wait.hpp"

constexpr int M = 20; // Number of actors
constexpr int N =  5; // Number of messages

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

MsgOptCont randSendMessage()
{
    for(int from = 0; from < M; ++from){
        int messageCount = std::rand() % N + 1;
        for (int i = 0; i < messageCount; ++i) {
            int recvAddr = randReciever(from);
            std::string message("Hello from Actor!");

            if (std::rand() % 2 == 0) {
                auto replyOpt = co_await actors[from]->send(recvAddr, 0, message, true);
                auto reply = replyOpt.value();
                printMessage("Actor %llu received reply for sequence number %d: %s\n", actors[from]->getAddress(), reply.respID, reply.content.c_str());
            }
            else {
                co_await actors[from]->send(recvAddr, 0, message);
            }
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

    auto t = randSendMessage();
    sync_wait(t);

    std::this_thread::sleep_for(std::chrono::seconds(1)); // Let the tasks finish

    actors.clear();
    pool.reset();

    printMessage("Gracefully exiting main\n");
    return 0;
}
