#pragma once

#include <string>

class Parser {
public:
    Parser();
    ~Parser();

    void parseSEND(char buffer[]);
    void parseLIST(char buffer[]);
    void parseREADorDEL(char buffer[], int index);

    std::string getSender() {return sender;};
    std::string getReceiver() {return receiver;};
    std::string getUsername() {return username;};
    std::string getSubject() {return subject;};
    std::string getMsg() {return msg;};
    int getMsgNum() {return msgNum;};



private:
    std::string sender;
    std::string receiver;
    std::string username;
    std::string subject;
    std::string msg;
    int msgNum;

};