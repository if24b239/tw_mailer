#include <iostream>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <Mail.hpp>

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
    
            std::cout << "Sender:";
            std::string sender = "";
            std::cin >> sender;
            //read input and clear whitespaces after
            std::cin.ignore();

            std::cout << "Receiver:";
            std::string receiver = "";
            std::cin >> receiver;
            //read input and clear whitespaces after
            std::cin.ignore();

            std::cout << "Subject:";
            std::string subject = "";
            std::cin >> subject;
            //read input and clear whitespaces after
            std::cin.ignore();

            std::cout << "Message:";
            std::string message = "\n";
            std::string line;
            //read line until . at start of line
            while (std::getline(std::cin, line) && !(line[0] == '.'))
            {
                message += line;
                message += "\n";
            }

            // create mail constuct
            Mail mail(sender, receiver, subject, message);

            // send the mail to the server
            clientSocket.sendMsg(mail, SEND);
        }
        else if (input == "LIST"){
            //read username from console
            std::cout << "Username:";   
            std::string username = "";
            std::cin >> username;

            clientSocket.sendMsg(username, LIST);
            
        }
        else if(input == "READ"){
            //read input from console
            std::cout << "Username:";
            std::string username = "";
            std::cin >> username;
            std::cout << "Message Number:";
            int msg_num;
            std::cin >> msg_num;
            
            clientSocket.sendMsg(username, msg_num, READ);
        }
        else if(input == "DEL"){
            //read input from console
            std::cout << "Username:";
            std::string username = "";
            std::cin >> username;
            std::cout << "Message Number:";
            int msg_num;
            std::cin >> msg_num;
            
            clientSocket.sendMsg(username, msg_num, DEL);
        }

        // wait for server response
        char buffer[1024] = {0};
        ssize_t recvd = recv(clientSocket.getDescriptor(), buffer, sizeof(buffer), 0);
        if (recvd == -1) {
            perror("recv error");
            continue; // wait for a new datapacket
        }
        if (recvd == 0) { // connection closed by server
            std::cout << "Server disconnected.\n";
            break;
        }

        json message = json::parse(buffer);

        if (message["receive_type"].get<int>() & REPLY) {
            perror("invalid receive type");
            continue;
        }

        std::cout << message["content"].get<std::string>();

    }

    return 0;
}