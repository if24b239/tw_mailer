#pragma once

#include <netinet/in.h>
#include <sys/socket.h>
#include <string>
#include <Mail.hpp>
#include <types.hpp>

class MailerSocket: private sockaddr_in {
public:
    MailerSocket();
    /* construct with an IPv4 address (in network byte order) and port */
    MailerSocket(in_addr_t addr, int port);
    MailerSocket(int port);
    ~MailerSocket();

    /* returns the socket descriptor for the constructed socket */
    inline int getDescriptor() { return socket_fd; };

    /* returns port */
    inline int getPort() { return ntohs(this->sin_port); }

    /* accessors for bind()/accept() callers */
    sockaddr* getSockAddr();
    const sockaddr* getSockAddr() const;
    inline socklen_t getSockAddrLen() const { return sizeof(sockaddr_in); }

    void sendMsg(Mail m, ReceiveType type);
    void sendMsg(std::string c, ReceiveType type);
    void sendMsg(std::string c, int i, ReceiveType type);

private:
    int socket_fd;
};