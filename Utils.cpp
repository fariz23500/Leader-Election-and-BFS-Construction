#include "Utils.h"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>

void setNonBlocking(int socket) {
    int flags = fcntl(socket, F_GETFL, 0);
    if (flags == -1) {
        std::cerr << "Error getting socket flags" << std::endl;
        return;
    }
    if (fcntl(socket, F_SETFL, flags | O_NONBLOCK) == -1) {
        std::cerr << "Error setting socket to non-blocking mode" << std::endl;
    }
}
