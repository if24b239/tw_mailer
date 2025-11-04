#include <iostream>
#include <sys/socket.h>

int main() {
    
    std::cout << "This is the server application." << std::endl;
    int socket_fd;
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    return 0;
}