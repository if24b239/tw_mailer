#pragma once
#include <netinet/in.h>
#include <string>


class MailerSocket: private sockaddr_in {
public:
    MailerSocket();
    MailerSocket(in_addr addr, int port);
    MailerSocket(int port);
    ~MailerSocket();

    /*returns the socket descriptor for the constructed socket*/
    inline int getDescriptor() { return socket_fd; };

    /*prints out the ip address and port*/
    void getIpAddress();

    /*returns port*/
    inline int getPort() { return ntohs(this->sin_port); }
private:
    
    int socket_fd;
};