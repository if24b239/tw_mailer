#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>

#include "utils/MailerSocket.hpp"

int main() {

    MailerSocket serverSocket = MailerSocket();

    if (bind(serverSocket.getDescriptor(), (sockaddr*) &serverSocket, sizeof(serverSocket)) == -1) {
        perror("bind error");
        exit(1);
    }

    while (listen(serverSocket.getDescriptor(), 1) != -1) {
        
    };

    return 0;
}