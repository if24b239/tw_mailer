#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>

#include "socket.h"

int main() {

    int socket_fd;
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    return 0;
}