#include "FindServer.h"
#include "Globals.h"
#include "Singleton.h"
#include "Client.h"
#include <sys/poll.h>
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <vector>

using namespace std;

void
BecomeServer()
{
    cout << "Assuming server role." << endl;
}

bool
SearchRemoteServer(const int _server_socket, FindServerParams &_findServerParams)
{
    /* Now poll for already existing server on the network */
    pollfd server_pollfds[1];
    memset(&server_pollfds[0], 0, sizeof(pollfd));
    server_pollfds[0].fd = _server_socket;
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
                num_bytes = recvfrom(_server_socket,
                                     buffer,
                                     sizeof(buffer),
                                     0,
                                     reinterpret_cast<sockaddr *>(&remote_server_addr),
                                     &remote_server_addrlen);
            }
            else if (server_pollfds[0].revents & POLLPRI)
            {
                num_bytes = recvfrom(_server_socket,
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
    if (num_bytes >= 17 && response == SERVER_HEADER)
    {
        char remote_server_name[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(remote_server_addr.sin_addr), remote_server_name, INET_ADDRSTRLEN);
        cout << "Data received from " << remote_server_name << ":" << ntohs(remote_server_addr.sin_port) << "<"
             << buffer << ">" << endl;
        return true;
    }
    else if (num_bytes == 0)
    {
        cout << "No data received." << endl;
        return false;
    }
    return false;
}

vector<string>
RequestClientList(const int _server_socket)
{
    vector<string> connected_clients;
    cout << "Requesting client list..." << endl;
    if (send(_server_socket, GET_CLIENT_LIST, sizeof(GET_CLIENT_LIST), 0) == -1)
    {
        cerr << "Failed to do handshake." << endl;
        return connected_clients;
    }
    ssize_t num_bytes = recv(_server_socket, buffer, sizeof(buffer) - 1, 0);
    if (num_bytes == -1)
    {
        cerr << "Error receiving data." << endl;
        return connected_clients;
    }
    return connected_clients;
}

void
UpdateClientList(const vector<string> &_connected_clients)
{
    for (auto it = _connected_clients.cbegin(); it != _connected_clients.cend(); ++it)
    {
        pthread_mutex_lock(&client_list_m);
        Singleton<std::map<string, Client> >::getInstance().operator[](*it).address = *it;
        pthread_mutex_unlock(&client_list_m);
    }
}

bool
EstablishConnection(const int _server_socket, FindServerParams &_findServerParams, const addrinfo &_server_addrinfo)
{
    if (connect(_server_socket, _server_addrinfo.ai_addr, _server_addrinfo.ai_addrlen) == -1)
    {
        if (errno == EISCONN)
        {
            return EXIT_SUCCESS;
        }
        cerr << "Failed to establish TCP connection to server." << endl;
        return EXIT_FAILURE;
    }

    sockaddr_in *server_sockaddr = reinterpret_cast< sockaddr_in *>(_server_addrinfo.ai_addr);
    char server_name[INET_ADDRSTRLEN];
    inet_ntop(_server_addrinfo.ai_family, &(server_sockaddr->sin_addr), server_name, INET_ADDRSTRLEN);
    cout << "Connected to " << server_name << ":" << ntohs(server_sockaddr->sin_port) << endl;
    if (send(_server_socket, CLIENT_HEADER, sizeof(CLIENT_HEADER), 0) == -1)
    {
        cerr << "Failed to do handshake." << endl;
        return EXIT_FAILURE;
    }

    pollfd server_pollfds[1];
    memset(&server_pollfds[0], 0, sizeof(pollfd));
    server_pollfds[0].fd = _server_socket;
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
                num_bytes = recv(_server_socket, buffer, sizeof(buffer) - 1, 0);
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
        char ip[INET_ADDRSTRLEN];
        inet_ntop(_server_addrinfo.ai_family, &(server_sockaddr->sin_addr), ip, sizeof(ip));
        Singleton<std::map<string, Client> >::getInstance().operator[](string(ip)).is_connected = true;
        Singleton<std::map<string, Client> >::getInstance().operator[](string(ip)).name =
            response.substr(sizeof(SERVER_REPLY));
        pthread_mutex_unlock(&client_list_m);
        return EXIT_SUCCESS;
    }

    return EXIT_FAILURE;
}

bool
SendClientList(const int _server_socket)
{
    if (send(_server_socket, &PUT_CLIENT_LIST, sizeof(PUT_CLIENT_LIST), 0) == -1)
    {
        cerr << "Failed to do send client list." << endl;
        return EXIT_FAILURE;
    }

    vector<Client> connected_clients;
    for (auto it = Singleton<std::map<string, Client> >::getInstance().cbegin();
         it != Singleton<std::map<string, Client> >::getInstance().cend(); ++it)
    {
        if (it->second.is_connected)
        {
            connected_clients.push_back(it->second);
        }
    }

    uint32_t size = htonl(static_cast<uint32_t>(connected_clients.size()));
    if (send(_server_socket, &size, sizeof(size), 0) == -1)
    {
        cerr << "Failed to do send client list." << endl;
        return EXIT_FAILURE;
    }

    for (auto it = connected_clients.cbegin(); it != connected_clients.cend(); ++it)
    {
        if (send(_server_socket, it->address.c_str(), sizeof(it->address.c_str()), 0) == -1)
        {
            cerr << "Failed to do send client list." << endl;
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}

void *
FindServer(void *_findServerParams)
{
    FindServerParams *findServerParams = reinterpret_cast<FindServerParams *>(_findServerParams);
    if (findServerParams == NULL)
    {
        cerr << "No server parameters passed?" << endl;
        return THREAD_RETURN(EXIT_FAILURE);
    }
    addrinfo hints, *results, *server_addrinfo = nullptr;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    int status = getaddrinfo("localhost", findServerParams->port, &hints, &results);
    if (status < 0)
    {
        cerr << "getaddrinfo() failed, " << gai_strerror(status) << endl;
        return THREAD_RETURN(EXIT_FAILURE);
    }

    int server_socket = -1;
    for (addrinfo *p = results; p != NULL; p = p->ai_next)
    {
        server_socket = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (server_socket < 0)
        {
            cerr << "Couldn't create socket, continuing..." << endl;
            continue;
        }

        int yes = 1;
        if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0)
        {
            close(server_socket);
            server_socket = -1;
            cerr << "setsockopt() failed to set SO_REUSEADDR, continuing..." << endl;
            continue;
        }

        if (setsockopt(server_socket, SOL_SOCKET, SO_BROADCAST, &yes, sizeof(yes)) < 0)
        {
            close(server_socket);
            server_socket = -1;
            cerr << "setsockopt() failed to set SO_BROADCAST, fatal..." << endl;
            close(server_socket);
            break;
        }

        if (bind(server_socket, p->ai_addr, p->ai_addrlen) < 0)
        {
            close(server_socket);
            server_socket = -1;
            cerr << "bind() failed, continuing..." << endl;
            continue;
        }
        server_addrinfo = p;
        sockaddr_in *server_sockaddr_in = reinterpret_cast< sockaddr_in *>(p->ai_addr);
        char server_name[INET_ADDRSTRLEN];
        inet_ntop(p->ai_family, &(server_sockaddr_in->sin_addr), server_name, INET_ADDRSTRLEN);
        cout << "Bound to socket at " << server_name << ":" << ntohs(server_sockaddr_in->sin_port) << endl;
        break;
    }

    if (server_socket == -1)
    {
        cerr << "bind() failed" << endl;
        return THREAD_RETURN(EXIT_FAILURE);
    }

    bool i_am_server = false;
    while (true_condition)
    {
        if (SearchRemoteServer(server_socket, *findServerParams))
        {
            if (!EstablishConnection(server_socket, *findServerParams, *server_addrinfo))
            {
                cerr << "Failed to connect, continuing..." << endl;
                continue;
            }
            vector<string> remote_client_list = RequestClientList();
            UpdateClientList(remote_client_list);
            vector<Client> connected_clients;
            for (auto it = Singleton<std::map<string, Client> >::getInstance().cbegin();
                 it != Singleton<std::map<string, Client> >::getInstance().cend(); ++it)
            {
                if (it->second.is_connected)
                {
                    connected_clients.push_back(it->second);
                }
            }

            if (i_am_server)
            {
                if (connected_clients.size() > remote_client_list.size())
                {
                    SendClientList(server_socket);
                }
                else
                {
                    InformClients();
                    KillServer();
                    i_am_server = false;
                }
            }
        }
        else
        {
            BecomeServer();
            i_am_server = true;
        }
    }
    freeaddrinfo(results);
    close(server_socket);
    return THREAD_RETURN(EXIT_SUCCESS);
}
