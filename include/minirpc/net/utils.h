#pragma once

#include <iostream>

namespace minirpc
{


int Dial(int port, const std::string& host = "127.0.0.1");


} // namespace minirpc