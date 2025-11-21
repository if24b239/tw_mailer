#include <iostream>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <Mail.hpp>
#include <termios.h>

#include "utils/MailerSocket.hpp"

void HideStdinKeystrokes()
{
    termios tty;

    tcgetattr(STDIN_FILENO, &tty);

    /* we want to disable echo */
    tty.c_lflag &= ~ECHO;

    tcsetattr(STDIN_FILENO, TCSANOW, &tty);
}

void ShowStdinKeystrokes()
{
   termios tty;

    tcgetattr(STDIN_FILENO, &tty);

    /* we want to reenable echo */
    tty.c_lflag |= ECHO;

    tcsetattr(STDIN_FILENO, TCSANOW, &tty);
}

int main(int argc, char* argv[]) {

    // Validate command line arguments
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <server_ip> <server_port>\n";
        return 1;
    }

    int port = std::stoi(argv[2]);
    in_addr addr;
    inet_aton(argv[1], &addr);

    bool loggedin = false;
    std::string username;

    MailerSocket clientSocket = MailerSocket(addr.s_addr , port);

    //connect
    if (connect(clientSocket.getDescriptor(), (sockaddr*) &clientSocket, sizeof(clientSocket)) == -1) {
        perror("connect error");
        exit(1);
    }

    std::string input = "";

    while ((std::cin >> input) && input != "QUIT") {
        if(input == "SEND") {
            if (!loggedin) {
                std::cout << "You need to be Logged In!\n";
                continue;
            }

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
            Mail mail(username, receiver, subject, message);

            // send the mail to the server
            clientSocket.sendMsg(mail, SEND);
        }
        else if (input == "LIST"){
            if (!loggedin) {
                std::cout << "You need to be Logged In!\n";
                continue;
            }
            //read mail subjects
            clientSocket.sendMsg(username, LIST);
            
        }
        else if(input == "READ"){
            if (!loggedin) {
                std::cout << "You need to be Logged In!\n";
                continue;
            }
            //read input from console
            std::cout << "Message Number:";
            int msg_num;
            std::cin >> msg_num;
            
            clientSocket.sendMsg(username, msg_num, READ);
        } else if (input == "DEL") {
            if (!loggedin) {
                std::cout << "You need to be Logged In!\n";
                continue;
            }
            //read input from console
            std::cout << "Message Number:";
            int msg_num;
            std::cin >> msg_num;
            
            clientSocket.sendMsg(username, msg_num, DEL);
        }
        else if (input == "LOGIN") {
            if (loggedin) {
                std::cout << "You are already Logged In as " + username + "\n";
                continue;
            }
            //read input from console
            std::cout << "Username:";
            std::cin >> username;
            std::cout << "Password:";
            std::string password = "";
            HideStdinKeystrokes();
            std::cin >> password;
            ShowStdinKeystrokes();
            std::cout << std::endl;

            clientSocket.sendMsg(username, password);
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
        if (!(message["receive_type"].get<int>() & REPLY)) {
            perror("invalid receive type");
            continue;
        }

        // if LOGIN was successful change state of loggedin
        if ((message["receive_type"].get<int>() & LOGIN) && !loggedin) {
            loggedin = message["content"] == "OK\n";
        }

        std::cout << message["content"].get<std::string>();

    }

    return 0;
}