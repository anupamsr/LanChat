#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <log4cxx/logger.h>
#include "Client.h"
#include "Globals.h"
#include "Singleton.h"
#include "Connect.h"

extern int
BindToSocketUDP(const char *_port, addrinfo &_server_addrinfo);

extern int
BindToSocketTCP(const char *_address, const char *_port, addrinfo &_server_addrinfo);

Client::Client()
    : address(), name(), socket(-1)
{}

Client::Client(const std::string & _address) : address(_address), name(), socket(-1)
{
}

void
StartClient()
{
    std::string chat_text;
    LOG4CXX_INFO(LanChatLogger::getLogger(), "On-line...");
    while (chat_text != "bye")
    {
        std::cout << "You: ";
        std::cin >> chat_text;
        if (chat_text[0] == '%' and chat_text.size() > 1)
        {
            std::string handle = chat_text.substr(1, chat_text.find(" "));
            int status = SendToClient(handle, chat_text);
            if (status == 0)
            {
                std::cout << "No client named: " << handle << std::endl;
            }
            else if (status < 0)
            {
                std::cout << "Could not send chat text to client" << std::endl;
            }
        }
        else
        {
            SendToServer(chat_text);
        }
    }
}

bool
SendToServer(std::string &_chat_text)
{
    if (SERVER.socket == -1)
    {
        LOG4CXX_ERROR(LanChatLogger::getLogger(), "Not connected to server");
        return false;
    }

    if (send(SERVER.socket, _chat_text.c_str(), sizeof(_chat_text.c_str()), 0) == -1)
    {
        std::ostringstream oss;
        oss << "send() failed: " << _chat_text;
        LOG4CXX_ERROR(LanChatLogger::getLogger(), oss.str());
        close(SERVER.socket);
        pthread_mutex_lock(&client_list_m);
        SERVER.socket = -1;
        pthread_mutex_unlock(&client_list_m);
        return false;
    }

    return true;
}

int
SendToClient(std::string &_handle, std::string &_chat_text)
{
    std::string address;
    for (auto it = Singleton<ClientMap>::getInstance().cbegin(); it != Singleton<ClientMap>::getInstance().cend(); ++it)
    {
        if (it->second.name == _handle)
        {
            address = it->second.address;
        }
    }

    if (address.empty())
    {
        LOG4CXX_ERROR(LanChatLogger::getLogger(), "Client not found");
        return 0;
    }

    Client client = Singleton<ClientMap>::getInstance().at(address);
    std::ostringstream oss;
    oss << "Sending chat text [" << _chat_text << "] to client " << client.address;
    LOG4CXX_INFO(LanChatLogger::getLogger(), oss.str());
    addrinfo _client_addrinfo;
    if (client.socket == -1)
    {
        client.socket = BindToSocketTCP(client.address.c_str(), PORT, _client_addrinfo);
    }

    if (client.socket == -1)
    {
        LOG4CXX_ERROR(LanChatLogger::getLogger(), "Could not connect to client");
        return -1;
    }

    if (connect(client.socket, _client_addrinfo.ai_addr, _client_addrinfo.ai_addrlen) == -1)
    {
        if (errno != EISCONN)
        {
            LOG4CXX_ERROR(LanChatLogger::getLogger(), "Failed to establish TCP connection to client");
            return -1;
        }
    }

    if (send(client.socket, _chat_text.c_str(), sizeof(_chat_text.c_str()), 0) == -1)
    {
        std::ostringstream oss;
        oss << "send() failed: " << _chat_text;
        LOG4CXX_ERROR(LanChatLogger::getLogger(), oss.str());
        close(client.socket);
        pthread_mutex_lock(&client_list_m);
        Singleton<ClientMap>::getInstance().operator[](_handle).socket = -1;
        pthread_mutex_unlock(&client_list_m);
        return false;
    }

    return 1;
}
