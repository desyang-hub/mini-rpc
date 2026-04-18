#pragma once

#include "minirpc/common/nonecopyable.h"


namespace minirpc
{
    



class EventLoop : public nonecopyable
{
private:
    /* data */
public:
    EventLoop(/* args */);
    ~EventLoop();
};


} // namespace minirpc