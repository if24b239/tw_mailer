#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <fstream>
#include <vector>

#include "utils/MailerSocket.hpp"
#include "utils/Parser.hpp"

int main(int argc, char* argv[]) {

    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << "<port> <directory_name>" << "\n";
        return 1;
    }

    std::string directory_name = argv[2];
    MailerSocket serverSocket = MailerSocket(std::stoi(argv[1]));

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
    

    sockaddr_in client_addr;
    socklen_t addrlen = sizeof(client_addr);
    for (;;) {
        // accept
        int new_sd = accept(serverSocket.getDescriptor(), (sockaddr*) &client_addr, &addrlen);
        if (new_sd == -1) {
            perror("accept error");
            continue; // try accepting again
        }

        std::cout << "Client connected.\n";

        // recv
        char buffer[1024] = {0};
        for (;;) {
            ssize_t recvd = recv(new_sd, buffer, sizeof(buffer), 0);
            if (recvd == -1) {
                perror("recv error");
                continue; // wait for a new datapacket
            }
            if (recvd == 0) { // connection closed by client
                std::cout << "Client disconnected.\n";
                break;
            }
            Parser parser = Parser();   //parser to correctly parse buffer for each method
            switch(buffer[0])   //chk first element of buffer (start of Prefix added in client)
            {
                case 'S':   //SEND
                {
                    //parse SEND buffer and get its elements
                    parser.parseSEND(buffer);
                    std::string sender = parser.getSender();
                    std::string receiver = parser.getReceiver();
                    std::string subject = parser.getSubject();
                    std::string msg = parser.getMsg();

                    //create or open specified file
                    std::ofstream file(directory_name + "/" + receiver + ".txt", std::ios::app);

                    //write info in file
                    file << "\nSender:" + sender + "\nReceiver:" + receiver + "\nSubject:" + subject + "\nMessage:" + msg;

                    file.close();

                    //TODO: error handling and send OK/ERR to client

                    break;
                }
                case 'L':   //LIST
                {
                    //parse LIST buffer and get its elements
                    parser.parseLIST(buffer);
                    std::string username = parser.getUsername();

                    std::string path = directory_name + "/" + username + ".txt";    //specifiy correct path to file

                        //chk whether file(username) exists
                        std::ifstream file(path);
                        if(!file) { std::cout << "unable to open file"; }
                        else
                        {
                            std::string line;
                            std::vector<std::string> subjects;

                            //add every line of file starting with "Subject:" to vector
                            while (getline(file, line)) {
                                if (line.find("Subject:") == 0) {
                                    std::string subject = line.substr(8);   //remove "Subject:"
                                    subjects.push_back(subject); 
                                }
                            }

                            //TODO: output to client not on console
                            std::cout << "Subjects count:" << subjects.size() << std::endl;
                            for (const auto& subject : subjects) {
                                std::cout << subject << std::endl;
                            }

                            file.close();
                        }

                    break;
                }
                case 'R':   //READ
                {
                    //parse READ buffer and get its elements
                    parser.parseREADorDEL(buffer, 6);
                    std::string username = parser.getUsername();

                    std::string path = directory_name + "/" + username + ".txt";    //specified filepath

                        //chk whether file(username) exists
                        std::ifstream file(path);
                        if(!file) { std::cout << "unable to open file"; }
                        else
                        {
                            //get remaining elements of buffer
                            int msgNum = parser.getMsgNum();

                            std::string line;
                            std::vector<std::string> msg;
                            int cnt = 0;

                            while (std::getline(file, line)) {
                                if (line.find("Sender:") == 0) {    //search for the msgNum.te element of "Sender:"
                                    cnt++;
                                    if (cnt == msgNum) {
                                        do
                                        {
                                            msg.push_back(line);    //add line +ff to vector
                                        } 
                                        while (std::getline(file, line) && line.find("Sender:") != 0);  //until next message starts (line again begins with "Sender:")

                                        break;
                                    }
                                }
                            }

                            //TODO: output to client not on console
                            for (const auto& m : msg) {
                                std::cout << m << std::endl;
                            }

                            file.close();
                        }

                    break;
                }
                case 'D':   //DEL
                {
                    parser.parseREADorDEL(buffer, 5);
                    break;
                }
            }

            //std::cout << "message from client: " << buffer << std::endl;
            std::memset(buffer, 0, sizeof(buffer)); //clear buffer after each message
        }
        close(new_sd);
    }


    return 0;
}