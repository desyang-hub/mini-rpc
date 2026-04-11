#include "common/timeStamp.h"

#include <time.h>

namespace common
{

TimeStamp::TimeStamp(int64_t micorSecondsSinceEpoch) : micorSecondsSinceEpoch_(micorSecondsSinceEpoch) {

}

TimeStamp TimeStamp::Now() {
    return TimeStamp{time(nullptr)};
}

std::string TimeStamp::toString() const {
    tm* ltm = localtime(&micorSecondsSinceEpoch_);

    char buf[128];

    snprintf(buf, 128, "%04d/%02d/%02d %02d:%02d:%02d", ltm->tm_year + 1900, ltm->tm_mon + 1, ltm->tm_mday, ltm->tm_hour, ltm->tm_min, ltm->tm_sec);

    return std::string(buf);
}


} // namespace common

