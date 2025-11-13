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
    int index = 6;

    for(int i = index; buffer[i] != ' '; i++)
    {
        this->sender.push_back(buffer[i]);
        index++;
    }

    index++;

    for(int i = index; buffer[i] != ' '; i++)
    {
        this->receiver.push_back(buffer[i]);
        index++;
    }

    index++;

    for(int i = index; buffer[i] != ' '; i++)
    {
        this->subject.push_back(buffer[i]);
        index++;
    }

    index++;

    for(int i = index; i < 1024; i++)
    {
        if(buffer[i] == NULL) break;
        this->msg.push_back(buffer[i]);
        index++;
    }
    
}

void Parser::parseLIST(char buffer[])
{
    int index = 6;

    for(int i = index; buffer[i] != NULL; i++)
    {
        this->username.push_back(buffer[i]);
        index++;
    }
}

void Parser::parseREADorDEL(char buffer[], int index)
{
    for(int i = index; buffer[i] != ' '; i++)
    {
        this->username.push_back(buffer[i]);
        index++;
    }

    index++;

    for(int i = index; buffer[i] != NULL; i++)
    {
        std::string num;
        num.push_back(buffer[i]);
        this->msgNum = stoi(num);
        index++;
    }
}