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

std::unique_ptr<ThreadPool> pool;
std::vector<std::unique_ptr<Actor>> actors;

int main(int argc, char **argv)
{
    std::srand(std::time(nullptr));
    pool = std::make_unique<ThreadPool>(argc > 1 ? std::stoi(argv[1]) : 4);

    for (int i = 0; i < M; ++i) {
        actors.push_back(std::make_unique<Actor>(*pool, i));
        pool->registerActor(actors.back().get());
    }

    std::string dummy;
    std::cin >> dummy;

    actors.clear();
    pool.reset();

    printMessage("Gracefully exiting main\n");
    return 0;
}
