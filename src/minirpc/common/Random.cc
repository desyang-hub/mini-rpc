#include "minirpc/common/Random.h"

#include <time.h>
#include <stdlib.h>

namespace minirpc
{

Random& Random::GetInstance() {
    static Random instance_;
    return instance_;
}

Random::Random() {
    srand(time(nullptr));
}

// rand number in [start, end)
int Random::randInt(int start, int end) {
    return start + rand() % (end - start);
}

int Random::RandInt(int start, int end) {
    return GetInstance().randInt(start, end);
}
    
} // namespace minirpc
