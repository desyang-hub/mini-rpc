#pragma once

#include "minirpc/common/nonecopyable.h"

namespace minirpc
{

class Poller : public nonecopyable
{
private:
    /* data */
public:
    Poller(/* args */);
    ~Poller();
};


} // namespace minirpc