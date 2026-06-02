#include "minirpc/common/Random.h"

#include <random>

namespace minirpc
{

Random& Random::GetInstance() {
    static Random instance_;
    return instance_;
}

Random::Random() : rng_(std::random_device{}()) {
}

int Random::randInt(int start, int end) {
    std::lock_guard<std::mutex> lock(mutex_);
    return std::uniform_int_distribution<int>(start, end - 1)(rng_);
}

int Random::RandInt(int start, int end) {
    return GetInstance().randInt(start, end);
}

} // namespace minirpc
