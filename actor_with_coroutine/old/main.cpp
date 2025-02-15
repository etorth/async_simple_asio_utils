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

constexpr int M = 20; // Number of actors

struct Message
{
    std::string content;
    int from;
    int seqID;
    int respID;

    Message(std::string content, int from, int seqID, int respID = 0)
        : content(std::move(content)), from(from), seqID(seqID), respID(respID) {}
};

class Actor;
class ThreadPool;

std::mutex coutMutex; // Mutex for synchronizing printf access

void print_message(const char *format, ...) {
    std::lock_guard<std::mutex> lock(coutMutex);
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    fflush(stdout); // Flush the output buffer
}

class Actor {
public:
    Actor(ThreadPool& pool, uint64_t address) : pool(pool), address(address), isProcessing(false), m_msgCount(0) {}

    uint64_t getAddress() const {
        return address;
    }

    void send(uint64_t toAddress, Message message, std::function<void(const Message&)> callback = nullptr);

    virtual void receive(const Message& message) {
        print_message("Actor %llu received message from Actor %d with content: %s, seqID: %d, respID: %s\n",
                      address, message.from, message.content.c_str(), message.seqID,
                      (message.respID != 0 ? std::to_string(message.respID).c_str() : "null"));

        m_lastMsg = message;
        m_msgCount++;

        if (message.respID != 0) {
            auto it = callbacks.find(message.respID);
            if (it != callbacks.end()) {
                it->second(m_lastMsg.value());
                callbacks.erase(it);
            } else {
                throw std::runtime_error("Callback not found for response ID: " + std::to_string(message.respID));
            }
        }

        // If the received message's seqID is non-zero, send a reply
        if (message.seqID != 0) {
            Message replyMessage("Reply from Actor!", address, 0, message.seqID);
            send(message.from, replyMessage);
        }
    }

    void consumeMessages();

    bool trySetProcessing() {
        return !isProcessing.exchange(true);
    }

    void resetProcessing() {
        isProcessing = false;
    }

    void sendMessages();

private:
    ThreadPool& pool;
    uint64_t address;
    std::vector<Message> mailbox;
    std::mutex mailboxMutex;
    std::atomic<bool> isProcessing;
    std::unordered_map<int, std::function<void(const Message&)>> callbacks;

    std::optional<Message> m_lastMsg;
    size_t m_msgCount;
    std::atomic<int> sequence{1}; // Start sequence from 1, as 0 will be used for messages that don't need a response
};

class ThreadPool {
public:
    ThreadPool(size_t numThreads) {
        for (size_t i = 0; i < numThreads; ++i) {
            workers.emplace_back([this] {
                while (true) {
                    std::function<void()> task;

                    {
                        std::unique_lock<std::mutex> lock(this->queueMutex);
                        this->condition.wait(lock, [this] { return this->stop || !this->tasks.empty(); });
                        if (this->stop && this->tasks.empty()) return;
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }

                    task();
                }
            });
        }
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            stop = true;
        }
        condition.notify_all();
        for (std::thread &worker : workers)
            worker.join();
    }

    void registerActor(std::shared_ptr<Actor> actor) {
        std::unique_lock<std::mutex> lock(actorsMutex);
        actors[actor->getAddress()] = actor;
    }

    std::shared_ptr<Actor> getActor(uint64_t address) {
        std::unique_lock<std::mutex> lock(actorsMutex);
        return actors[address];
    }

    void enqueue(std::function<void()> task) {
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            if (stop) throw std::runtime_error("enqueue on stopped ThreadPool");
            tasks.push(std::move(task));
        }
        condition.notify_one();
    }

    void scheduleActor(std::shared_ptr<Actor> actor) {
        enqueue([actor, this]() {
            if (actor->trySetProcessing()) {
                actor->consumeMessages();
                actor->resetProcessing();
            }
        });
    }

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queueMutex;
    std::condition_variable condition;
    bool stop = false;

    std::unordered_map<uint64_t, std::shared_ptr<Actor>> actors;
    std::mutex actorsMutex;
};

void Actor::send(uint64_t toAddress, Message message, std::function<void(const Message&)> callback) {
    auto target = pool.getActor(toAddress);
    if (target) {
        // If the callback is nullptr, set seqID to 0
        if (callback == nullptr) {
            message.seqID = 0;
        } else {
            // Store the callback in the hash table
            callbacks[message.seqID] = callback;
        }

        {
            std::unique_lock<std::mutex> lock(target->mailboxMutex);
            target->mailbox.push_back(message);
        }

        pool.scheduleActor(target);
    }
}

void Actor::consumeMessages() {
    while (true) {
        std::vector<Message> messages;
        {
            std::unique_lock<std::mutex> lock(mailboxMutex);
            std::swap(messages, mailbox);
        }
        if (messages.empty()) break;
        for (auto& message : messages) {
            receive(message);
        }
    }
}

void Actor::sendMessages() {
    int messageCount = std::rand() % 5 + 1;

    for (int i = 0; i < messageCount; ++i) {
        uint64_t randomActorAddress;
        do {
            randomActorAddress = std::rand() % M + 1;
        } while (randomActorAddress == getAddress());

        int sequenceNumber = sequence++;
        Message message("Hello from Actor!", address, sequenceNumber);

        // Randomly decide whether to provide a callback or not
        if (std::rand() % 2 == 0) {
            send(randomActorAddress, message, [this, sequenceNumber](const Message& reply) {
                print_message("Actor %llu received reply for sequence number %d: %s\n",
                              getAddress(), sequenceNumber, reply.content.c_str());
            });
        } else {
            send(randomActorAddress, message);
        }
    }
}

int main() {
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

    print_message("Gracefully exiting main\n");
    return 0;
}
