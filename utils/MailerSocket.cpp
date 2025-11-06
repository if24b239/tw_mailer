#include <MailerSocket.hpp>
#include <memory.h>
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>

MailerSocket::MailerSocket(in_addr& addr, int port) {
    try {
        this->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (this->socket_fd == -1) {
            throw "Could not create socket";
        }

        this->sin_family = AF_INET;
        this->sin_addr = addr;
        this->sin_port = htons(port);

    } catch (const char* msg) {
        // Handle socket creation error
        perror(msg);
    }
};

MailerSocket::MailerSocket(int port) {
    try {
        this->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (this->socket_fd == -1) {
            throw "Could not create socket";
        }

        this->sin_family = AF_INET;
        this->sin_addr.s_addr = htonl(INADDR_ANY);
        this->sin_port = htons(port);

    } catch (const char* msg) {
        // Handle socket creation error
        perror(msg);
    }

    std::cout << "Socket created with descriptor: " << this->socket_fd << std::endl;
}

MailerSocket::MailerSocket() : MailerSocket(6000) {
    // Default constructor calls parameterized constructor with port 6000
}

MailerSocket::~MailerSocket() {
    close(this->socket_fd);
    std::cout << "Socket with descriptor " << this->socket_fd << " closed." << std::endl;
}

void MailerSocket::getIpAddress() {
    system("curl ifconfig.me");
    std::cout << ":" << this->getPort() << std::endl;
}