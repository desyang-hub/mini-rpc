#pragma once

namespace minirpc
{

// 随机数产生类
class Random
{
private:
    static Random& GetInstance();
public:
    Random();

    static int RandInt(int start, int end);

    int randInt(int start, int end);
};
    
} // namespace minirpc
