#include <iostream>
#include <unistd.h>
#include <string.h>
#include <string>
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
            std::string message = "";
            std::string line;
            //read line until . at start of line
            while (std::getline(std::cin, line) && !(line[0] == '.'))
            {
                message += line;
            }
            //create msgString with identifying Prefix and char* [] to use in send
            std::string msgInput = "SEND: " + sender + " " + receiver + " " + subject + " " + message;
            const char* msg = msgInput.c_str();
            if (send(clientSocket.getDescriptor(), msg, strlen(msg), 0) == -1) {
                perror("send error");
                exit(1);
            }
        }
        else if (input == "LIST"){
            //read username from console
            std::cout << "Username:";   
            std::string username = "";
            std::cin >> username;
            //create msgString with identifying Prefix and char* [] to use in send
            std::string msgInput = "LIST: " + username;
            const char* msg = msgInput.c_str();
            if (send(clientSocket.getDescriptor(), msg, strlen(msg), 0) == -1) {
                perror("send error");
                exit(1);
            }
        }
        else if(input == "READ"){
            //read input from console
            std::cout << "Username:";
            std::string username = "";
            std::cin >> username;
            std::cout << "Messsage Number:";
            std::string msg_num = "";
            std::cin >> msg_num;
            //create msgString with identifying Prefix and char* [] to use in send
            std::string msgString = "READ: " + username + " " + msg_num;
            const char* msg = msgString.c_str();
            if (send(clientSocket.getDescriptor(), msg, strlen(msg), 0) == -1) {
                perror("send error");
                exit(1);
            }
        }
        else if(input == "DEL"){
            //read input from console
            std::cout << "Username:";
            std::string username = "";
            std::cin >> username;
            std::cout << "Messsage Number:";
            std::string msg_num = "";
            std::cin >> msg_num;
            //create msgString with identifying Prefix and char* [] to use in send
            std::string msgString = "DEL: " + username + " " + msg_num;
            const char* msg = msgString.c_str();
            if (send(clientSocket.getDescriptor(), msg, strlen(msg), 0) == -1) {
                perror("send error");
                exit(1);
            }
        }
    }

    return 0;
}