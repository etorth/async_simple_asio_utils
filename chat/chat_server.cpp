#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <algorithm>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <coroutine>

constexpr int MAX_CLIENTS = 10;
constexpr int BUFFER_SIZE = 1024;

std::mutex clients_mutex;
std::vector<int> clients;

void broadcast_message(const std::string& message, int sender_fd) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    for (int client_fd : clients) {
        if (client_fd != sender_fd) {
            send(client_fd, message.c_str(), message.size(), 0);
        }
    }
}

struct Awaitable {
    int fd;
    char buffer[BUFFER_SIZE];
    std::coroutine_handle<> handle;

    bool await_ready() { return false; }
    void await_suspend(std::coroutine_handle<> h) { handle = h; }
    void await_resume() {
        memset(buffer, 0, BUFFER_SIZE);
        recv(fd, buffer, BUFFER_SIZE, 0);
    }

    void operator()() { handle.resume(); }
};

struct Task {
    struct promise_type {
        Task get_return_object() { return {}; }
        std::suspend_never initial_suspend() { return {}; }
        std::suspend_never final_suspend() noexcept { return {}; }
        void return_void() {}
        void unhandled_exception() {}
    };
};

Task handle_client(int client_fd) {
    while (true) {
        Awaitable awaitable{client_fd};
        co_await awaitable;

        std::string message(awaitable.buffer);
        if (message.empty()) break;

        broadcast_message(message, client_fd);
    }

    std::lock_guard<std::mutex> lock(clients_mutex);
    clients.erase(std::remove(clients.begin(), clients.end(), client_fd), clients.end());
    close(client_fd);
}

void start_server(short port) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    bind(server_fd, (sockaddr*)&server_addr, sizeof(server_addr));
    listen(server_fd, MAX_CLIENTS);

    std::cout << "Server started on port " << port << std::endl;

    while (true) {
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);

        std::lock_guard<std::mutex> lock(clients_mutex);
        clients.push_back(client_fd);

        std::thread(handle_client, client_fd).detach();
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: chat_server <port>\n";
        return 1;
    }

    short port = std::stoi(argv[1]);
    start_server(port);

    return 0;
}
