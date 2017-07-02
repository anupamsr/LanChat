#ifndef LANCHAT_CLIENT_H
#define LANCHAT_CLIENT_H

#include <string>
#include <map>

struct Client
{
    std::string address;
    std::string name;
    int socket;

    Client();
    Client(const std::string &_address);
};

void
StartClient();

bool
SendToServer(std::string &_chat_text);

int
SendToClient(std::string &_handle, std::string &_chat_text);

#endif //LANCHAT_CLIENT_H
