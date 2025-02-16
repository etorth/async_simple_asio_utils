#pragma once
#include <string>

struct Message
{
    std::string content {};

    int from   = 0;
    int seqID  = 0;
    int respID = 0;
};
