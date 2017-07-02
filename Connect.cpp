#include <cstring>
#include <arpa/inet.h>
#include <sstream>
#include <unistd.h>
#include <log4cxx/logger.h>
#include "Connect.h"
#include "Singleton.h"

int
BindToSocketUDP(const char *_port, addrinfo &_remote_addrinfo)
{
    int server_fd = -1;
    addrinfo hints, *results;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    int status = getaddrinfo("localhost", _port, &hints, &results);
    if (status < 0)
    {
        LOG4CXX_ERROR(LanChatLogger::getLogger(), "getaddrinfo() failed");
        LOG4CXX_ERROR(LanChatLogger::getLogger(), gai_strerror(status));
        return server_fd;
    }

    int yes = 1;
    for (addrinfo *p = results; p != NULL; p = p->ai_next)
    {
        server_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (server_fd < 0)
        {
            LOG4CXX_INFO(LanChatLogger::getLogger(), "Couldn't create socket, continuing...");
            continue;
        }

        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0)
        {
            close(server_fd);
            server_fd = -1;
            LOG4CXX_INFO(LanChatLogger::getLogger(), "setsockopt() failed to set SO_REUSEADDR, continuing...");
            continue;
        }

        if (setsockopt(server_fd, SOL_SOCKET, SO_BROADCAST, &yes, sizeof(yes)) < 0)
        {
            close(server_fd);
            server_fd = -1;
            LOG4CXX_ERROR(LanChatLogger::getLogger(),
                          "setsockopt() failed to set SO_BROADCAST, stopping further search");
            close(server_fd);
            break;
        }

        if (bind(server_fd, p->ai_addr, p->ai_addrlen) < 0)
        {
            close(server_fd);
            server_fd = -1;
            LOG4CXX_INFO(LanChatLogger::getLogger(), "bind() failed, continuing...");
            continue;
        }

        memcpy(p, &_remote_addrinfo, sizeof(p));

        sockaddr_in *server_sockaddr_in = reinterpret_cast<sockaddr_in *>(p->ai_addr);
        char server_name[INET_ADDRSTRLEN];
        inet_ntop(p->ai_family, &(server_sockaddr_in->sin_addr), server_name, INET_ADDRSTRLEN);
        std::ostringstream oss;
        oss << "Bound to UDP socket at " << server_name << ":" << ntohs(server_sockaddr_in->sin_port);
        LOG4CXX_INFO(LanChatLogger::getLogger(), oss.str());
        break;
    }

    freeaddrinfo(results);
    return server_fd;
}

int
BindToSocketTCP(const char *_address, const char *_port, addrinfo &_remote_addrinfo)
{
    int server_fd = -1;
    addrinfo hints, *results;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int status = getaddrinfo(_address, _port, &hints, &results);
    if (status < 0)
    {
        std::ostringstream oss;
        oss << "getaddrinfo() failed for " << _address << ":" << _port << ":" << gai_strerror(status);
        LOG4CXX_ERROR(LanChatLogger::getLogger(), oss.str());
        return server_fd;
    }

    int yes = 1;
    for (addrinfo *p = results; p != NULL; p = p->ai_next)
    {
        server_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (server_fd < 0)
        {
            LOG4CXX_INFO(LanChatLogger::getLogger(), "Couldn't create socket, continuing...");
            continue;
        }

        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0)
        {
            close(server_fd);
            server_fd = -1;
            LOG4CXX_INFO(LanChatLogger::getLogger(), "setsockopt() failed to set SO_REUSEADDR, continuing...");
            continue;
        }

        if (bind(server_fd, p->ai_addr, p->ai_addrlen) < 0)
        {
            close(server_fd);
            server_fd = -1;
            LOG4CXX_INFO(LanChatLogger::getLogger(), "bind() failed, continuing...");
            continue;
        }

        memcpy(p, &_remote_addrinfo, sizeof(p));

        sockaddr_in *server_sockaddr_in = reinterpret_cast<sockaddr_in *>(p->ai_addr);
        char server_name[INET_ADDRSTRLEN];
        inet_ntop(p->ai_family, &(server_sockaddr_in->sin_addr), server_name, INET_ADDRSTRLEN);
        std::ostringstream oss;
        oss << "Bound to TCP socket at " << server_name << ":" << ntohs(server_sockaddr_in->sin_port);
        LOG4CXX_INFO(LanChatLogger::getLogger(), oss.str());
        break;
    }

    freeaddrinfo(results);
    return server_fd;
}
