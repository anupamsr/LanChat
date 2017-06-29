#ifndef LANCHAT_CLIENT_H
#define LANCHAT_CLIENT_H

#include <string>
#include <map>

struct Client
{
    std::string address;
    std::string name;
    bool is_connected;

    Client();
};


#endif //LANCHAT_CLIENT_H
