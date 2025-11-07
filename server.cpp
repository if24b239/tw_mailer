#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>

#include "utils/MailerSocket.hpp"

int main() {

    MailerSocket serverSocket = MailerSocket(8080);

    std::cout << "Server IP Address and Port " << std::endl;
    serverSocket.getIpAddress();

    if (bind(serverSocket.getDescriptor(), (sockaddr*) &serverSocket, sizeof(serverSocket)) == -1) {
        perror("bind error");
        exit(1);
    }

    while (listen(serverSocket.getDescriptor(), 1) != -1) {
        //accept
        sockaddr_in client_addr;
        socklen_t addrlen = sizeof(client_addr);
        int new_sd;

        try {
        new_sd = accept(serverSocket.getDescriptor(),(sockaddr*) &client_addr, &addrlen);  
            if (new_sd == -1) {
                throw "Could not accept connection";
            }
        } catch (const char* msg) {
            perror(msg);
        }

        //recv
        char buffer[1024] = {0};
        if (recv(new_sd, buffer, sizeof(buffer), 0) == -1) {
            perror("recv error");
            exit(1);
        }
        std::cout << "message from client: " << buffer << std::endl;
    };


    return 0;
}