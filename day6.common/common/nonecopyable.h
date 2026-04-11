#pragma once

namespace common
{
class nonecopyable
{
public:
    nonecopyable(const nonecopyable&) = delete;
    nonecopyable& operator=(const nonecopyable&) = delete;

protected:
    nonecopyable() = default;
};

} // namespace moduo