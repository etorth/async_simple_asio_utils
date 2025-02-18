#include <iostream>
#include <string>
#include <thread>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <coroutine>

constexpr int BUFFER_SIZE = 1024;

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

Task receive_messages(int client_fd) {
    while (true) {
        Awaitable awaitable{client_fd};
        co_await awaitable;

        std::string message(awaitable.buffer);
        if (message.empty()) break;

        std::cout << "Received: " << message << std::endl;
    }

    close(client_fd);
}

void start_client(const std::string& host, short port) {
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, host.c_str(), &server_addr.sin_addr);

    connect(client_fd, (sockaddr*)&server_addr, sizeof(server_addr));

    std::thread(receive_messages, client_fd).detach();

    std::string message;
    while (std::getline(std::cin, message)) {
        send(client_fd, message.c_str(), message.size(), 0);
    }

    close(client_fd);
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: chat_client <host> <port>\n";
        return 1;
    }

    std::string host = argv[1];
    short port = std::stoi(argv[2]);
    start_client(host, port);

    return 0;
}
