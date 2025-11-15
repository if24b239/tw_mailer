#include <Parser.hpp>
#include <string>

Parser::Parser()
{
    sender = "";
    receiver = "";
    username = "";
    subject = "";
    msg = "";
}

Parser::~Parser()
{

}

void Parser::parseSEND(char buffer[])
{
    int index = 6;  //length of prefix "SEND: "

    //first entry of buffer write into sender
    for(int i = index; buffer[i] != ' '; i++)   //stops at first ' '
    {
        this->sender.push_back(buffer[i]);
        index++;
    }

    index++;    //increase index inbetween to start again at start of new entry

    //second entry receiver
    for(int i = index; buffer[i] != ' '; i++)   //stops at first ' '
    {
        this->receiver.push_back(buffer[i]);
        index++;
    }

    index++;

    //third entry subject
    for(int i = index; buffer[i] != ' '; i++)   //stops at first ' '
    {
        this->subject.push_back(buffer[i]);
        index++;
    }

    index++;

    int length = 1024 - index;  //remaining max length of buffer
    //fourth entry msg
    for(int i = index; i < length; i++)
    {
        if(buffer[i] == 0) break;    //stops at first NULL element
        this->msg.push_back(buffer[i]);
        index++;
    }
    
}

void Parser::parseLIST(char buffer[])
{
    int index = 6;  //length of prefix "LIST: "

    //first element is username
    for(int i = index; buffer[i] != 0; i++)    //stops at first NULL element
    {
        this->username.push_back(buffer[i]);
        index++;
    }
}

void Parser::parseREADorDEL(char buffer[], int index)   //index as parameter because only difference of function between READ and DEL
{
    //first element is username
    for(int i = index; buffer[i] != ' '; i++)   //stops at first ' '
    {
        this->username.push_back(buffer[i]);
        index++;
    }

    index++;    //increase index inbetween to start again at start of new entry

    //second element is msg number
    for(int i = index; buffer[i] != 0; i++)  //stops at first NULL element
    {
        std::string num;
        num.push_back(buffer[i]);
        this->msgNum = stoi(num);
        index++;
    }
}