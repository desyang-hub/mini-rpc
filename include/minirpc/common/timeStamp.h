#pragma once

#include <iostream>
#include <string>

namespace minirpc
{

class TimeStamp
{
public:
    TimeStamp() = default;
    explicit TimeStamp(int64_t micorSecondsSinceEpoch);

    static TimeStamp Now();

    std::string toString() const;

private:
    int64_t micorSecondsSinceEpoch_{0};
};

    
} // namespace minirpc
