#include "FindServer.h"
#include "Globals.h"
#include "Singleton.h"
#include <sys/poll.h>
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <vector>

using namespace std;

static Client &
GetClient(const char *_ip)
{
    return Singleton<ClientMap>::getInstance().operator[](string(_ip));
}

int
BecomeServer(const int _server_udp_fd)
{
    cout << "Broadcasting..." << endl;
    struct hostent *broadcast_gateway;
    if ((broadcast_gateway = gethostbyname("255.255.255.255")) == NULL)
    {
        cerr << "Couldn't get broadcast address..." << endl;
        return EXIT_FAILURE;
    }
    sockaddr_in broadcast_sockaddr;
    memset(&broadcast_sockaddr, 0, sizeof(broadcast_sockaddr));
    broadcast_sockaddr.sin_family = AF_INET;
    broadcast_sockaddr.sin_port = htons(2000);
    broadcast_sockaddr.sin_addr = *((struct in_addr *) broadcast_gateway->h_addr);
    ssize_t num_byes = -1;
    if (sendto(_server_udp_fd,
               &SERVER_HEADER,
               sizeof(SERVER_HEADER),
               0,
               reinterpret_cast<sockaddr *>(&broadcast_sockaddr),
               sizeof(broadcast_sockaddr)) == -1)
    {
        cerr << "Couldn't broadcast..." << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

void
InformClients()
{
    cout << "Updating clients of closing server..." << endl;
    for (auto it = Singleton<ClientMap>::getInstance().begin(); it != Singleton<ClientMap>::getInstance().end(); ++it)
    {
        if (it->second.socket != -1)
        {
            if (send(it->second.socket, &SERVER_CLOSING, sizeof(SERVER_CLOSING), 0) == -1)
            {
                cerr << "Failed to update client, marking offline" << endl;
                close(it->second.socket);
                it->second.socket = -1;
            }
        }
    }
}

bool
SearchRemoteServer(const int _server_fd, FindServerParams &_findServerParams)
{
    /* Poll for already existing server on the network */
    pollfd server_pollfds[1];
    memset(&server_pollfds[0], 0, sizeof(pollfd));
    server_pollfds[0].fd = _server_fd;
    server_pollfds[0].events = POLLIN | POLLPRI;
    int status =
        poll(server_pollfds, sizeof(server_pollfds) / sizeof(server_pollfds[0]), _findServerParams.duration * 1000);

    /* Read data from server, if available */
    char buffer[_findServerParams.bufLen];
    sockaddr_in remote_server_addr;
    socklen_t remote_server_addrlen;
    ssize_t num_bytes = -1;
    switch (status)
    {
        case -1:
            cerr << "poll() failed, " << status << endl;
            break;
        case 0:
            cerr << "Timeout of " << _findServerParams.duration << " seconds expired." << endl;
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
                cerr << "Remote server ended, continuing..." << endl;
            }
            break;
    }

    /* We ignore data that doesn't interest us. */
    string response = buffer;
    if (num_bytes >= sizeof(SERVER_HEADER) && response == SERVER_HEADER)
    {
        char remote_server_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(remote_server_addr.sin_addr), remote_server_ip, INET_ADDRSTRLEN);
        cout << "Data received from " << remote_server_ip << ":" << ntohs(remote_server_addr.sin_port) << "<" << buffer
             << ">" << endl;
        return true;
    }
    else if (num_bytes == 0)
    {
        cout << "No data received." << endl;
    }
    return false;
}

bool
EstablishConnection(int _server_tcp_fd, FindServerParams &_findServerParams, const addrinfo &_server_addrinfo)
{
    sockaddr_in *server_sockaddr = reinterpret_cast< sockaddr_in *>(_server_addrinfo.ai_addr);
    char ip[INET_ADDRSTRLEN];
    inet_ntop(_server_addrinfo.ai_family, &(server_sockaddr->sin_addr), ip, sizeof(ip));
    if (connect(_server_tcp_fd, _server_addrinfo.ai_addr, _server_addrinfo.ai_addrlen) == -1)
    {
        if (errno == EISCONN && GetClient(ip).socket == _server_tcp_fd)
        {
            return EXIT_SUCCESS;
        }
        cerr << "Failed to establish TCP connection to server." << endl;
        return EXIT_FAILURE;
    }

    cout << "Connected to " << ip << ":" << ntohs(server_sockaddr->sin_port) << endl;
    if (send(_server_tcp_fd, CLIENT_HEADER, sizeof(CLIENT_HEADER), 0) == -1)
    {
        cerr << "Failed to do handshake." << endl;
        return EXIT_FAILURE;
    }

    /* Poll for handshake acknowledgement */
    pollfd server_pollfds[1];
    memset(&server_pollfds[0], 0, sizeof(pollfd));
    server_pollfds[0].fd = _server_tcp_fd;
    server_pollfds[0].events = POLLIN;
    int status =
        poll(server_pollfds, sizeof(server_pollfds) / sizeof(server_pollfds[0]), _findServerParams.duration * 1000);

    /* Read data from server, if available */
    char buffer[_findServerParams.bufLen];
    ssize_t num_bytes = -1;
    switch (status)
    {
        case -1:
            cerr << "poll() failed, " << status << endl;
            break;
        case 0:
            cerr << "Timeout of " << _findServerParams.duration << " seconds expired." << endl;
            break;
        default:
            if (server_pollfds[0].revents & POLLIN)
            {
                num_bytes = recv(_server_tcp_fd, buffer, sizeof(buffer) - 1, 0);
            }
            else if (server_pollfds[0].revents & POLLHUP)
            {
                cerr << "Remote server ended, continuing..." << endl;
            }
            break;
    }

    /* We ignore data that doesn't interest us. */
    string response = buffer;
    if (num_bytes > sizeof(SERVER_REPLY) && response == SERVER_REPLY)
    {
        pthread_mutex_lock(&client_list_m);
        GetClient(ip).socket = _server_tcp_fd;
        GetClient(ip).name = response.substr(sizeof(SERVER_REPLY));
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
        cerr << "Failed to do send client list." << endl;
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
        cerr << "Failed to do send client list." << endl;
        return EXIT_FAILURE;
    }

    for (auto it = connected_clients.cbegin(); it != connected_clients.cend(); ++it)
    {
        if (send(_server_tcp_fd, it->address.c_str(), sizeof(it->address.c_str()), 0) == -1)
        {
            cerr << "Failed to do send client list." << endl;
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}

vector<string>
RequestClientList(const int _server_tcp_fd)
{
    vector<string> connected_clients;
    cout << "Requesting client list..." << endl;
    if (send(_server_tcp_fd, GET_CLIENT_LIST, sizeof(GET_CLIENT_LIST), 0) == -1)
    {
        cerr << "Failed to make request." << endl;
        return connected_clients;
    }

    uint32_t size;
    if (recv(_server_tcp_fd, &size, sizeof(size), 0) == -1)
    {
        cerr << "Error receiving response." << endl;
        return connected_clients;
    }

    size = ntohl(size);
    for (auto i = 0; i < size; ++i)
    {
        char buffer[MAX_BUF_LEN];
        if (recv(_server_tcp_fd, &buffer, sizeof(buffer) - 1, 0) == -1)
        {
            cerr << "Error receiving data." << endl;
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

int
BindToSocketUDP(const FindServerParams &_findServerParams, addrinfo &_server_addrinfo)
{
    int server_fd = -1;
    addrinfo hints, *results;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    int status = getaddrinfo("localhost", _findServerParams.port, &hints, &results);
    if (status < 0)
    {
        cerr << "getaddrinfo() failed, " << gai_strerror(status) << endl;
        return server_fd;
    }

    int yes = 1;
    for (addrinfo *p = results; p != NULL; p = p->ai_next)
    {
        server_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (server_fd < 0)
        {
            cerr << "Couldn't create socket, continuing..." << endl;
            continue;
        }

        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0)
        {
            close(server_fd);
            server_fd = -1;
            cerr << "setsockopt() failed to set SO_REUSEADDR, continuing..." << endl;
            continue;
        }

        if (setsockopt(server_fd, SOL_SOCKET, SO_BROADCAST, &yes, sizeof(yes)) < 0)
        {
            close(server_fd);
            server_fd = -1;
            cerr << "setsockopt() failed to set SO_BROADCAST, fatal..." << endl;
            close(server_fd);
            break;
        }

        if (bind(server_fd, p->ai_addr, p->ai_addrlen) < 0)
        {
            close(server_fd);
            server_fd = -1;
            cerr << "bind() failed, continuing..." << endl;
            continue;
        }

        sockaddr_in *server_sockaddr_in = reinterpret_cast<sockaddr_in *>(p->ai_addr);
        char server_name[INET_ADDRSTRLEN];
        inet_ntop(p->ai_family, &(server_sockaddr_in->sin_addr), server_name, INET_ADDRSTRLEN);
        cout << "Bound to UDP socket at " << server_name << ":" << ntohs(server_sockaddr_in->sin_port) << endl;
        memcpy(p, &_server_addrinfo, sizeof(p));
        break;
    }

    freeaddrinfo(results);
    return server_fd;
}

int
BindToSocketTCP(const FindServerParams &_findServerParams)
{
    int server_fd = -1;
    addrinfo hints, *results;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int status = getaddrinfo("localhost", _findServerParams.port, &hints, &results);
    if (status < 0)
    {
        cerr << "getaddrinfo() failed, " << gai_strerror(status) << endl;
        return server_fd;
    }

    int yes = 1;
    for (addrinfo *p = results; p != NULL; p = p->ai_next)
    {
        server_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (server_fd < 0)
        {
            cerr << "Couldn't create socket, continuing..." << endl;
            continue;
        }

        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0)
        {
            close(server_fd);
            server_fd = -1;
            cerr << "setsockopt() failed to set SO_REUSEADDR, continuing..." << endl;
            continue;
        }

        if (bind(server_fd, p->ai_addr, p->ai_addrlen) < 0)
        {
            close(server_fd);
            server_fd = -1;
            cerr << "bind() failed, continuing..." << endl;
            continue;
        }

        sockaddr_in *server_sockaddr_in = reinterpret_cast<sockaddr_in *>(p->ai_addr);
        char server_name[INET_ADDRSTRLEN];
        inet_ntop(p->ai_family, &(server_sockaddr_in->sin_addr), server_name, INET_ADDRSTRLEN);
        cout << "Bound to TCP socket at " << server_name << ":" << ntohs(server_sockaddr_in->sin_port) << endl;
        break;
    }

    freeaddrinfo(results);
    return server_fd;
}

void *
FindServer(void *_findServerParams)
{
    FindServerParams *findServerParams = reinterpret_cast<FindServerParams *>(_findServerParams);
    if (findServerParams == NULL)
    {
        cerr << "No server parameters passed?" << endl;
        THREAD_EXIT(EXIT_FAILURE);
    }

    addrinfo server_addrinfo;
    int server_udp_fd = BindToSocketUDP(*findServerParams, server_addrinfo);
    if (server_udp_fd == -1)
    {
        cerr << "bind() failed" << endl;
        THREAD_EXIT(EXIT_FAILURE);
    }

    bool i_am_server = false;
    while (true)
    {
        if (SearchRemoteServer(server_udp_fd, *findServerParams))
        {
            int server_tcp_fd = BindToSocketTCP(*findServerParams);
            if (server_tcp_fd != -1 && !EstablishConnection(server_tcp_fd, *findServerParams, server_addrinfo))
            {
                cerr << "Failed to connect, continuing..." << endl;
                continue;
            }
            vector<string> remote_client_list = RequestClientList(server_tcp_fd);
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

            if (i_am_server)
            {
                if (connected_clients.size() > remote_client_list.size())
                {
                    SendClientList(server_tcp_fd);
                }
                else
                {
                    InformClients();
                    i_am_server = false;
                }
            }
        }
        else
        {
            if (BecomeServer(server_udp_fd) == EXIT_SUCCESS)
            {
                i_am_server = true;
            }
            sleep(findServerParams->duration * 0.5);
        }
    }

    close(server_udp_fd);
    THREAD_EXIT(EXIT_SUCCESS);
}
