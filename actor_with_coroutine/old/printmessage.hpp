#pragma once
#include <cstdio>
#include <cstdarg>
#include <mutex>

inline void printMessage(const char *format, ...)
{
    static std::mutex coutMutex;
    const  std::lock_guard<std::mutex> lock(coutMutex);
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    fflush(stdout); // Flush the output buffer
}
