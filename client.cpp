#include <iostream>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>

#include "utils/MailerSocket.hpp"

int main(int argc, char* argv[]) {

    // Validate command line arguments
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <server_ip> <server_port>\n";
        return 1;
    }

    int port = std::stoi(argv[2]);
    in_addr addr;
    inet_aton(argv[1], &addr);

    MailerSocket clientSocket = MailerSocket(addr.s_addr , port);

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
            std::getline(std::cin >> std::ws, msgInput);
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