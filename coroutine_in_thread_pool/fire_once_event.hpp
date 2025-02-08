#pragma once

#include <atomic>

struct fire_once_event
{
    void set()
    {
        m_flag.test_and_set();
        m_flag.notify_all();
    }

    void wait()
    {
        m_flag.wait(false);
    }
private:
    std::atomic_flag m_flag;
};
