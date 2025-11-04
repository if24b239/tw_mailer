#pragma once
#include <netinet/in.h>

class MailerSocket: private sockaddr_in {
public:

    MailerSocket();
    ~MailerSocket();
private:
    /*returns the socket descriptor for the constructed socket*/
    int getDescriptor();
};