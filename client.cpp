#include <iostream>
#include <unistd.h>
#include <string.h>

#include "utils/MailerSocket.hpp"

int main() {
    
    MailerSocket clientSocket = MailerSocket();

    //connect
    if (connect(clientSocket.getDescriptor(), (sockaddr*) &clientSocket, sizeof(clientSocket)) == -1) {
        perror("connect error");
        exit(1);
    }

    sleep(5);

    //send
    const char* msg = "Hello!";
    if (send(clientSocket.getDescriptor(), msg, strlen(msg), 0) == -1) {
        perror("send error");
        exit(1);
    }

    return 0;
}