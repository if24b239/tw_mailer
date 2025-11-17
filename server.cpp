#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <cstring>
#include <fstream>
#include <vector>

#include "utils/MailerSocket.hpp"

void saveMessage(json mail, std::string dir) {
    std::string path = dir + "/" + mail["receiver"].get<std::string>() + ".txt";
    
    // Read existing data
    std::ifstream infile(path);
    json data;
    if (infile.good()) {
        std::string content((std::istreambuf_iterator<char>(infile)), std::istreambuf_iterator<char>());
        infile.close();
        if (!content.empty()) {
            data = json::parse(content);
        } else {
            data = json::array();
        }
    } else {
        data = json::array();
    }
    
    // Add new message
    data.push_back(mail);
    
    // Write back
    std::ofstream outfile(path);
    if (!outfile) { std::cout << "unable to open file: " << path << std::endl; return; }
    outfile << data.dump(4);
    outfile.close();
}


std::vector<std::string> listMessages(std::string username, std::string dir) {
    
    std::string path = dir + "/" + username + ".txt"; //specifiy correct path to file
    std::ifstream file(path); //check whether file(username) exists
    if(!file) { std::cout << "unable to open file: " << path << std::endl; }

    // get the json data and list all subjects
    json data = json::parse(file);
    
    file.close();

    std::vector<std::string> subjects;
    for (const auto& message : data) {
        subjects.push_back(message["subject"].get<std::string>());
    }

    return subjects;
}

Mail returnMessage(std::string username, int number, std::string dir) {

    std::string path = dir + "/" + username + ".txt"; //specifiy correct path to file
    std::ifstream file(path); //check whether file(username) exists
    if(!file) { std::cout << "unable to open file: " << path << std::endl; }

    // get the json data and return the right message
    json data = json::parse(file);

    file.close();

    return data.at(number-1).get<Mail>();
}

void deleteMessage(std::string username, int number, std::string dir) {
    
    std::string path = dir + "/" + username + ".txt"; //specifiy correct path to file
    std::ifstream infile(path, std::ios::out);
    if(!infile) { std::cout << "unable to open file: " << path << std::endl; }

    json data;
    if (infile.good()) {
        std::string content((std::istreambuf_iterator<char>(infile)), std::istreambuf_iterator<char>());
        infile.close();
        if (!content.empty()) {
            data = json::parse(content);
        } else {
            std::cout << "No Messages to be deleted in file: " << path << std::endl;
            return;
        }
    } else {
        std::cout << "unable to open file: " << path << std::endl;
    }
    infile.close();

    data.erase(data.begin() + number-1);

    std::ofstream outfile(path);
    if(!outfile) { std::cout << "unable to open file: " << path << std::endl; }
    outfile << data.dump(4);

    outfile.close();
}

int main(int argc, char* argv[]) {

    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <port> <directory_name>" << "\n";
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
        int client_sd = accept(serverSocket.getDescriptor(), (sockaddr*) &client_addr, &addrlen);
        if (client_sd == -1) {
            perror("accept error");
            continue; // try accepting again
        }
        std::cout << "Client connected.\n";

        // recv
        char buffer[1024] = {0};
        for (;;) {
            ssize_t recvd = recv(client_sd, buffer, sizeof(buffer), 0);
            if (recvd == -1) {
                perror("recv error");
                continue; // wait for a new datapacket
            }
            if (recvd == 0) { // connection closed by client
                std::cout << "Client disconnected.\n";
                break;
            }

            json message = json::parse(buffer);

            json returnMsg = json::object();

            std::string c;
            
            switch (message["receive_type"].get<int>())
            {
            case SEND:
                saveMessage(message["mail"], directory_name);
                c = "OK\n";
                break;
            case LIST:
                {
                    int count = 0;
                    std::string subjects = "";
                    for (auto s : listMessages(message["content"].get<std::string>(), directory_name)) {
                        subjects += s;
                        subjects += "\n";
                        count++;
                    }
                    ;
                    c += "Count of Messages: " + std::to_string(count) + '\n';
                    c += subjects;
                    std::cout << c;
                }
                break;
            case READ:
                c = returnMessage(message["content"].get<std::string>(), message["number"].get<int>(), directory_name).serialize();
                break;
            case DEL:
                deleteMessage(message["content"].get<std::string>(), message["number"].get<int>(), directory_name);
                c = "OK\n";
                break;
            
            default:
                perror("INVALID RETURN JSON");
                c = "ERR\n";
                break;
            }
            std::memset(buffer, 0, sizeof(buffer)); //clear buffer after each message

            nlohmann::json j;
            j["receive_type"] = ReceiveType::REPLY;
            j["content"] = c;
            std::string msg = j.dump();
            const char* messag = msg.c_str();
            if (send(client_sd, messag, strlen(messag), 0) == -1) {
                perror("send error");
                exit(1);
            }
        }
        close(client_sd);
    }


    return 0;
}