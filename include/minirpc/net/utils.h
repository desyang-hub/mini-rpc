#pragma once

#include <iostream>
#include <vector>

namespace minirpc
{

void set_nonblocking(int fd);

int Dial(int port, const std::string& host = "127.0.0.1");

// void register_services(const std::vector<std::string>& services, int server_port, const std::string& host="127.0.0.1");

} // namespace minirpc