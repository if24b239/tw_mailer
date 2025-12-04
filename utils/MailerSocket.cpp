#include <MailerSocket.hpp>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <json.hpp>

MailerSocket::MailerSocket(in_addr_t addr, int port) {
    try {
        this->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (this->socket_fd == -1) {
            throw "Could not create socket";
        }

        /* allow quick reuse during development */
        int yes = 1;
        setsockopt(this->socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

        /* initialize the sockaddr_in base subobject to zero */
        std::memset(static_cast<sockaddr_in*>(this), 0, sizeof(sockaddr_in));

        this->sin_family = AF_INET;
        this->sin_addr.s_addr = addr; // addr is expected in network byte order
        this->sin_port = htons(port);

    } catch (const char* msg) {
        // Handle socket creation error
        perror(msg);
    }

    std::cout << "Socket created with descriptor: " << this->socket_fd << std::endl;
}

MailerSocket::MailerSocket(int port) : MailerSocket(htonl(INADDR_ANY), port) {
    // Parameterized constructor to create a socket bound to any incoming address on the specified port
}

MailerSocket::MailerSocket() : MailerSocket(6000) {
    // Default constructor calls parameterized constructor with port 6000
}

MailerSocket::~MailerSocket() {
    close(this->socket_fd);
    std::cout << "Socket with descriptor " << this->socket_fd << " closed." << std::endl;
}

sockaddr* MailerSocket::getSockAddr() {
    return reinterpret_cast<sockaddr*>(static_cast<sockaddr_in*>(this));
}

const sockaddr* MailerSocket::getSockAddr() const {
    return reinterpret_cast<const sockaddr*>(static_cast<const sockaddr_in*>(this));
}

void sendMessage(std::string msg, int socket) {
    size_t msgLength = msg.size();
    size_t totalSent = 0;

    const char* message = msg.c_str();

    while(totalSent < msgLength) {
        ssize_t sent = send(socket, message, msgLength - totalSent, 0);
        if (sent == -1) {
            perror("send error");
            exit(1);
        }
        totalSent += sent;
    }
}

/*
if (send(socket, message, strlen(message), 0) == -1) {
        perror("send error");
        exit(1);


size_t totalSent = 0;
    size_t messageLength = message.size();

    while (totalSent < messageLength) {
        ssize_t sent = send(socket, message.data() + totalSent, messageLength - totalSent, 0);
        if (sent == -1) {
            perror("send error");
            break; // Handle the error appropriately
        }
        totalSent += sent;
    }
*/

void MailerSocket::sendMsg(Mail m, ReceiveType type) {
    nlohmann::json j;
    j["receive_type"] = type;
    j["mail"] = m;
    sendMessage(j.dump(), this->getDescriptor());
}

void MailerSocket::sendMsg(std::string c, ReceiveType type) {
    nlohmann::json j;
    j["receive_type"] = type;
    j["content"] = c;
    sendMessage(j.dump(), this->getDescriptor());
}

void MailerSocket::sendMsg(std::string c, int i, ReceiveType type) {
    nlohmann::json j;
    j["receive_type"] = type;
    j["content"] = c;
    j["number"] = i;
    sendMessage(j.dump(), this->getDescriptor());
}

void MailerSocket::sendMsg(std::string u, std::string p) {
    nlohmann::json j;
    j["receive_type"] = LOGIN;
    j["content"] = json::object();
    j["content"]["username"] = u;
    j["content"]["password"] = p;
    sendMessage(j.dump(), this->getDescriptor());
}
