#pragma once
#include <netinet/in.h>

class MailerSocket: private sockaddr_in {
public:
    MailerSocket();
    MailerSocket(int port);
    ~MailerSocket();

    /*returns the socket descriptor for the constructed socket*/
    inline int getDescriptor() { return socket_fd; };

    /*returns the address struct for the constructed socket*/
private:
    
    int socket_fd;
};