#include <iostream>
#include <unistd.h>
#include <string.h>
#include <string>

#include "utils/MailerSocket.hpp"

int main() {

    MailerSocket clientSocket = MailerSocket(8080);

    //connect
    if (connect(clientSocket.getDescriptor(), (sockaddr*) &clientSocket, sizeof(clientSocket)) == -1) {
        perror("connect error");
        exit(1);
    }

    std::string input = "";

    while ((std::cin >> input) && input != "QUIT") {
        if(input == "SEND") {
            std::cout << "Send:";
            std::string msgInput = "";
            std::cin >> msgInput;
            const char* msg = msgInput.c_str();
            if (send(clientSocket.getDescriptor(), msg, strlen(msg), 0) == -1) {
                perror("send error");
                exit(1);
            }
        }
    }

    //list
    //read
    //del

    return 0;
}