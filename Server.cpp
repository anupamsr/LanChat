#include "Server.h"
#include <cstring>
#include <poll.h>
#include <arpa/inet.h>
#include <log4cxx/logger.h>
#include "Globals.h"
#include "Singleton.h"

using namespace std;

extern int
BindToSocketUDP(const char *_port, addrinfo &_server_addrinfo);

extern int
BindToSocketTCP(const char *_address, const char *_port, addrinfo &_server_addrinfo);

int
BecomeServer(const int _server_udp_fd)
{
    LOG4CXX_INFO(LanChatLogger::getLogger(), "Broadcasting...");
    struct hostent *broadcast_gateway;
    if ((broadcast_gateway = gethostbyname("255.255.255.255")) == NULL)
    {
        LOG4CXX_ERROR(LanChatLogger::getLogger(), "Couldn't get broadcast address");
        return EXIT_FAILURE;
    }
    sockaddr_in broadcast_sockaddr;
    memset(&broadcast_sockaddr, 0, sizeof(broadcast_sockaddr));
    broadcast_sockaddr.sin_family = AF_INET;
    broadcast_sockaddr.sin_port = htons(2000);
    broadcast_sockaddr.sin_addr = *((struct in_addr *) broadcast_gateway->h_addr);
    if (sendto(_server_udp_fd,
               &SERVER_HEADER,
               sizeof(SERVER_HEADER),
               0,
               reinterpret_cast<sockaddr *>(&broadcast_sockaddr),
               sizeof(broadcast_sockaddr)) == -1)
    {
        LOG4CXX_ERROR(LanChatLogger::getLogger(), "Couldn't broadcast");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

void
StopServer()
{
    LOG4CXX_INFO(LanChatLogger::getLogger(), "Updating clients of closing server...");
    for (auto it = Singleton<ClientMap>::getInstance().begin(); it != Singleton<ClientMap>::getInstance().end(); ++it)
    {
        if (it->second.socket != -1)
        {
            if (send(it->second.socket, &SERVER_CLOSING, sizeof(SERVER_CLOSING), 0) == -1)
            {
                ostringstream oss;
                oss << "Failed to update client " << it->second.address << "(" << it->second.name << ")"
                    << " marking offline";
                LOG4CXX_ERROR(LanChatLogger::getLogger(), oss.str());
                close(it->second.socket);
                pthread_mutex_lock(&client_list_m);
                it->second.socket = -1;
                pthread_mutex_unlock(&client_list_m);
            }
        }
    }
}

bool
SearchRemoteServer(const int _server_fd, const int _duration, const size_t _buffer_len)
{
    /* Poll for already existing server on the network */
    pollfd server_pollfds[1];
    memset(&server_pollfds[0], 0, sizeof(pollfd));
    server_pollfds[0].fd = _server_fd;
    server_pollfds[0].events = POLLIN | POLLPRI;
    int status =
        poll(server_pollfds, sizeof(server_pollfds) / sizeof(server_pollfds[0]), _duration * 1000);

    /* Read data from server, if available */
    ostringstream oss;
    char buffer[_buffer_len];
    sockaddr_in remote_server_addr;
    socklen_t remote_server_addrlen;
    ssize_t num_bytes = -1;
    switch (status)
    {
        case -1:
        LOG4CXX_ERROR(LanChatLogger::getLogger(), "poll() failed");
            break;
        case 0:
            oss << "Timeout of " << _duration << " seconds expired";
            LOG4CXX_ERROR(LanChatLogger::getLogger(), oss.str());
            break;
        default:
            if (server_pollfds[0].revents & POLLIN)
            {
                num_bytes = recvfrom(_server_fd,
                                     buffer,
                                     sizeof(buffer),
                                     0,
                                     reinterpret_cast<sockaddr *>(&remote_server_addr),
                                     &remote_server_addrlen);
            }
            else if (server_pollfds[0].revents & POLLPRI)
            {
                num_bytes = recvfrom(_server_fd,
                                     buffer,
                                     sizeof(buffer),
                                     MSG_OOB,
                                     reinterpret_cast<sockaddr *>(&remote_server_addr),
                                     &remote_server_addrlen);
            }
            else if (server_pollfds[0].revents & POLLHUP)
            {
                LOG4CXX_ERROR(LanChatLogger::getLogger(), "Remote server ended");
            }
            break;
    }

    /* We ignore data that doesn't interest us. */
    string response = buffer;
    if (num_bytes >= sizeof(SERVER_HEADER) && response == SERVER_HEADER)
    {
        char remote_server_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(remote_server_addr.sin_addr), remote_server_ip, INET_ADDRSTRLEN);
        oss.str("");
        oss << "Data received from " << remote_server_ip << ":" << ntohs(remote_server_addr.sin_port) << ":" << buffer;
        LOG4CXX_INFO(LanChatLogger::getLogger(), oss.str());
        return true;
    }
    else if (num_bytes == 0)
    {
        LOG4CXX_INFO(LanChatLogger::getLogger(), "No data received");
    }
    return false;
}

bool
ConnectToServer(int _server_tcp_fd, const int _duration, const size_t _buffer_len, const addrinfo &_server_addrinfo)
{
    sockaddr_in *server_sockaddr = reinterpret_cast< sockaddr_in *>(_server_addrinfo.ai_addr);
    char ip[INET_ADDRSTRLEN];
    inet_ntop(_server_addrinfo.ai_family, &(server_sockaddr->sin_addr), ip, sizeof(ip));
    if (connect(_server_tcp_fd, _server_addrinfo.ai_addr, _server_addrinfo.ai_addrlen) == -1)
    {
        if (errno == EISCONN && SERVER.socket == _server_tcp_fd)
        {
            return EXIT_SUCCESS;
        }
        LOG4CXX_ERROR(LanChatLogger::getLogger(), "Failed to establish TCP connection to server.");
        return EXIT_FAILURE;
    }

    ostringstream oss;
    oss << "Connected to " << ip << ":" << ntohs(server_sockaddr->sin_port);
    LOG4CXX_INFO(LanChatLogger::getLogger(), oss.str());
    if (send(_server_tcp_fd, CLIENT_HEADER, sizeof(CLIENT_HEADER), 0) == -1)
    {
        LOG4CXX_ERROR(LanChatLogger::getLogger(), "Failed to do handshake.");
        return EXIT_FAILURE;
    }

    /* Poll for handshake acknowledgement */
    pollfd server_pollfds[1];
    memset(&server_pollfds[0], 0, sizeof(pollfd));
    server_pollfds[0].fd = _server_tcp_fd;
    server_pollfds[0].events = POLLIN;
    int status =
        poll(server_pollfds, sizeof(server_pollfds) / sizeof(server_pollfds[0]), _duration * 1000);

    /* Read data from server, if available */
    char buffer[_buffer_len];
    ssize_t num_bytes = -1;
    switch (status)
    {
        case -1:
        LOG4CXX_ERROR(LanChatLogger::getLogger(), "poll() failed");
            break;
        case 0:
            oss.str("");
            oss << "Timeout of " << _duration << " seconds expired";
            LOG4CXX_ERROR(LanChatLogger::getLogger(), oss.str());
            break;
        default:
            if (server_pollfds[0].revents & POLLIN)
            {
                num_bytes = recv(_server_tcp_fd, buffer, sizeof(buffer) - 1, 0);
            }
            else if (server_pollfds[0].revents & POLLHUP)
            {
                LOG4CXX_ERROR(LanChatLogger::getLogger(), "Remote server ended");
            }
            break;
    }

    /* We ignore data that doesn't interest us. */
    string response = buffer;
    if (num_bytes > sizeof(SERVER_REPLY) && response.substr(0, sizeof(SERVER_REPLY)) == SERVER_REPLY)
    {
        pthread_mutex_lock(&client_list_m);
        SERVER.socket = _server_tcp_fd;
        SERVER.address = string(ip);
        SERVER.name = response.substr(sizeof(SERVER_REPLY), response.size() - sizeof(SERVER_REPLY));
        Singleton<ClientMap>::getInstance().insert(pair<string, Client>(ip, SERVER));
        pthread_mutex_unlock(&client_list_m);
        return EXIT_SUCCESS;
    }

    return EXIT_FAILURE;
}

bool
SendClientList(const int _server_tcp_fd)
{
    if (send(_server_tcp_fd, &PUT_CLIENT_LIST, sizeof(PUT_CLIENT_LIST), 0) == -1)
    {
        LOG4CXX_ERROR(LanChatLogger::getLogger(), "Failed to send client list(1)");
        return EXIT_FAILURE;
    }

    vector<Client> connected_clients;
    for (auto it = Singleton<ClientMap>::getInstance().cbegin();
         it != Singleton<ClientMap>::getInstance().cend(); ++it)
    {
        if (it->second.socket != -1)
        {
            connected_clients.push_back(it->second);
        }
    }

    uint32_t size = htonl(static_cast<uint32_t>(connected_clients.size()));
    if (send(_server_tcp_fd, &size, sizeof(size), 0) == -1)
    {
        LOG4CXX_ERROR(LanChatLogger::getLogger(), "Failed to do send client list(2)");
        return EXIT_FAILURE;
    }

    for (auto it = connected_clients.cbegin(); it != connected_clients.cend(); ++it)
    {
        if (send(_server_tcp_fd, it->address.c_str(), sizeof(it->address.c_str()), 0) == -1)
        {
            LOG4CXX_ERROR(LanChatLogger::getLogger(), "Failed to do send client list(3)");
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}

vector<string>
RequestClientList(const int _tcp_fd)
{
    LOG4CXX_INFO(LanChatLogger::getLogger(), "Requesting client list");
    vector<string> connected_clients;
    if (send(_tcp_fd, GET_CLIENT_LIST, sizeof(GET_CLIENT_LIST), 0) == -1)
    {
        LOG4CXX_ERROR(LanChatLogger::getLogger(), "Failed to make request");
        return connected_clients;
    }

    uint32_t size;
    if (recv(_tcp_fd, &size, sizeof(size), 0) == -1)
    {
        LOG4CXX_ERROR(LanChatLogger::getLogger(), "Error receiving response");
        return connected_clients;
    }

    size = ntohl(size);
    for (auto i = 0; i < size; ++i)
    {
        char buffer[MAX_BUF_LEN];
        if (recv(_tcp_fd, &buffer, sizeof(buffer) - 1, 0) == -1)
        {
            LOG4CXX_ERROR(LanChatLogger::getLogger(), "Error receiving data");
            break;
        }
        buffer[sizeof(buffer) - 1] = '\0';
        connected_clients.push_back(string(buffer));
    }
    return connected_clients;
}

void
UpdateClientList(const vector<string> &_connected_clients)
{
    for (auto it = _connected_clients.cbegin(); it != _connected_clients.cend(); ++it)
    {
        pthread_mutex_lock(&client_list_m);
        Singleton<ClientMap>::getInstance().operator[](*it).address = *it;
        pthread_mutex_unlock(&client_list_m);
    }
}

void *
ManageServer(void *_manage_server_params)
{
    ManageServerParams *manage_server_params = reinterpret_cast<ManageServerParams *>(_manage_server_params);
    if (manage_server_params == NULL)
    {
        LOG4CXX_FATAL(LanChatLogger::getLogger(), "No server parameters passed?");
        THREAD_EXIT(EXIT_FAILURE);
    }

    addrinfo server_addrinfo;
    int server_udp_fd = BindToSocketUDP(manage_server_params->port, server_addrinfo);
    if (server_udp_fd == -1)
    {
        LOG4CXX_FATAL(LanChatLogger::getLogger(), "bind() failed");
        THREAD_EXIT(EXIT_FAILURE);
    }

    bool i_am_server = false;
    while (true)
    {
        if (SearchRemoteServer(server_udp_fd, manage_server_params->duration, manage_server_params->buf_len))
        {
            int server_tcp_fd = BindToSocketTCP(SERVER.address.c_str(), manage_server_params->port, server_addrinfo);
            if (server_tcp_fd != -1 &&
                !ConnectToServer(server_tcp_fd,
                                 manage_server_params->duration,
                                 manage_server_params->buf_len,
                                 server_addrinfo))
            {
                LOG4CXX_ERROR(LanChatLogger::getLogger(), "Failed to connect, continuing...");
                continue;
            }
        }
        else
        {
            if (BecomeServer(server_udp_fd) == EXIT_SUCCESS)
            {
                BindToSocketTCP("localhost", manage_server_params->port, server_addrinfo);
                i_am_server = true;
            }
        }

        vector<string> remote_client_list = RequestClientList(SERVER.socket);
        UpdateClientList(remote_client_list);
        vector<Client> connected_clients;
        for (auto it = Singleton<ClientMap>::getInstance().cbegin();
             it != Singleton<ClientMap>::getInstance().cend(); ++it)
        {
            if (it->second.socket != -1)
            {
                connected_clients.push_back(it->second);
            }
        }

        SendClientList(SERVER.socket);
        if (connected_clients.size() > remote_client_list.size())
        {
            StopServer();
            i_am_server = false;
        }

        if (i_am_server)
        {
            usleep(manage_server_params->duration * 500);
        }
    }

    close(server_udp_fd);
    THREAD_EXIT(EXIT_SUCCESS);
}
