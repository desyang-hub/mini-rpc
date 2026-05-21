#include "minirpc/net/utils.h"

#include <fcntl.h>

namespace minirpc
{

void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl get error");
        return;
    }
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

} // namespace minirpc
