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

#include "message.hpp"
#include "printmessage.hpp"
#include "threadpool.hpp"
#include "actor.hpp"

int main()
{
    constexpr int M = 20; // Number of actors
    ThreadPool pool(4);
    std::vector<std::shared_ptr<Actor>> actors;

    for (int i = 0; i < M; ++i) {
        auto actor = std::make_shared<Actor>(pool, i + 1);
        actors.push_back(actor);
        pool.registerActor(actor);
    }

    for (auto& actor : actors) {
        actor->sendMessages();
    }

    std::this_thread::sleep_for(std::chrono::seconds(1)); // Let the tasks finish

    // Clear actors
    actors.clear();

    printMessage("Gracefully exiting main\n");
    return 0;
}
