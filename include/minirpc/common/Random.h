#pragma once

#include <random>
#include <mutex>

namespace minirpc
{

class Random
{
public:
    Random();

    static int RandInt(int start, int end);

private:
    static Random& GetInstance();
    int randInt(int start, int end);

    std::mt19937 rng_;
    std::mutex mutex_;
};

} // namespace minirpc
