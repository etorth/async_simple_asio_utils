#include <iostream>
#include <string>
#include <cstdint>
#include "corof.hpp"

corof::eval_poller<> k()
{
    std::cout << "begin wait" << std::endl;
    hres_timer t;
    co_await corof::async_wait(1000);
    std::cout << "end   wait: " << t.diff_secf() << std::endl;
}

corof::eval_poller<int> f()
{
    co_await k();
    co_return 12;
}

corof::eval_poller<std::string> g()
{
    co_return std::string("7");
}

// corof::eval_poller<const char *> g()
// {
//     co_return "7";
// }

corof::eval_poller<int> r()
{
    const auto cf = co_await f();
    const auto cg = co_await g();

    co_return cf + std::stoi(cg);
}

int main()
{
    auto p = r();
    std::cout << p.sync_eval() << std::endl;
    return 10;
}
