#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include "utils/MailerSocket.hpp"

int main() {

    MailerSocket serverSocket = MailerSocket(8080);

    if (bind(serverSocket.getDescriptor(), serverSocket.getSockAddr(), serverSocket.getSockAddrLen()) == -1) {
        perror("bind error");
        exit(1);
    }

    if (listen(serverSocket.getDescriptor(), SOMAXCONN) == -1) {
        perror("listen error");
        exit(1);
    }

    std::cout << "Server listening on port " << serverSocket.getPort() << "\n";

    for (;;) {
        // accept
        sockaddr_in client_addr;
        socklen_t addrlen = sizeof(client_addr);
        int new_sd = accept(serverSocket.getDescriptor(), (sockaddr*) &client_addr, &addrlen);
        if (new_sd == -1) {
            perror("accept error");
            continue; // try accepting again
        }

        // recv
        char buffer[1024] = {0};
        ssize_t recvd = recv(new_sd, buffer, sizeof(buffer), 0);
        if (recvd == -1) {
            perror("recv error");
            close(new_sd);
            continue;
        }
        std::cout << "message from client: " << buffer << std::endl;
        close(new_sd); //TODO: client QUIT should close the connection.
    }


    return 0;
}