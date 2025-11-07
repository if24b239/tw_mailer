#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "utils/MailerSocket.hpp"

int main() {

    MailerSocket serverSocket = MailerSocket(8080);

    if (bind(serverSocket.getDescriptor(), serverSocket.getSockAddr(), serverSocket.getSockAddrLen()) == -1) {
        perror("bind error");
        exit(1);
    }

    if (listen(serverSocket.getDescriptor(), SOMAXCONN) == -1) {
        perror("listen error");
        exit(1);
    }

    /* Binding and listening info */
    
    system("echo -n 'Local IP addr: ';ip addr show | grep 'eth0' | grep 'inet' | awk '{print $2}' | cut -d'/' -f1");

    sockaddr_in addr;
    socklen_t len = sizeof(addr);
    if (getsockname(serverSocket.getDescriptor(), (sockaddr*)&addr, &len) == 0) {
        char s[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &addr.sin_addr, s, sizeof(s));
        std::cout << "Bound to " << s << "\n";
    }
    std::cout << "Server listening on port " << serverSocket.getPort() << "\n";
    

    for (;;) {
        // accept
        sockaddr_in client_addr;
        socklen_t addrlen = sizeof(client_addr);
        int new_sd = accept(serverSocket.getDescriptor(), (sockaddr*) &client_addr, &addrlen);
        if (new_sd == -1) {
            perror("accept error");
            continue; // try accepting again
        }

        // recv
        char buffer[1024] = {0};
        ssize_t recvd = recv(new_sd, buffer, sizeof(buffer), 0);
        if (recvd == -1) {
            perror("recv error");
            close(new_sd);
            continue;
        }
        std::cout << "message from client: " << buffer << std::endl;
        close(new_sd); //TODO: client QUIT should close the connection.
    }


    return 0;
}