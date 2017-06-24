#include "FindServer.h"

#include <sys/poll.h>
#include <arpa/inet.h>
#include <cstring>
#include <string>
#include <iostream>

using namespace std;

#define THREAD_EXIT(retVal) return reinterpret_cast<void *>(retVal);

void *
FindServer(void *_findServerParams)
{
    FindServerParams *findServerParams = reinterpret_cast<FindServerParams *>(_findServerParams);
    addrinfo hints, *server_addrinfo;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    int status = getaddrinfo("localhost", findServerParams->port, &hints, &server_addrinfo);
    if (status < 0)
    {
        cerr << "getaddrinfo() failed, " << gai_strerror(status) << endl;
        THREAD_EXIT(EXIT_FAILURE);
    }

    int server_socket = -1;
    for (addrinfo *p = server_addrinfo; p != NULL; p = p->ai_next)
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

//        timeval duration;
//        duration.tv_sec = TIMEOUT_DURATION;
//        duration.tv_usec = 0;
//        if (setsockopt(server_socket, SOL_SOCKET, SO_RCVTIMEO, &duration, sizeof(duration)) < 0)
//        {
//            close(server_socket);
//            server_socket = -1;
//            cerr << "setsockopt() failed to set SO_RCVTIMEO, continuing..." << endl;
//            continue;
//        }

        if (bind(server_socket, p->ai_addr, p->ai_addrlen) < 0)
        {
            close(server_socket);
            server_socket = -1;
            cerr << "bind() failed, continuing..." << endl;
            continue;
        }
        sockaddr_in *server_sockaddr = reinterpret_cast< sockaddr_in *> (p->ai_addr);
        char server_name[INET_ADDRSTRLEN];
        inet_ntop(p->ai_family, &(server_sockaddr->sin_addr), server_name, INET_ADDRSTRLEN);
        cout << "Bound to socket at " << server_name << ":" << ntohs(server_sockaddr->sin_port) << endl;
        break;
    }

    freeaddrinfo(server_addrinfo);
    if (server_socket == -1)
    {
        cerr << "bind() failed" << endl;
        THREAD_EXIT(EXIT_FAILURE);
    }

    /* Now poll for already existing server on the network */
    pollfd server_pollfds[1];
    memset(&server_pollfds[0], 0, sizeof(pollfd));
    server_pollfds[0].fd = server_socket;
    server_pollfds[0].events = POLLIN | POLLPRI;
    status =
        poll(server_pollfds, sizeof(server_pollfds) / sizeof(server_pollfds[0]), findServerParams->duration * 1000);

    /* Read data from server, if available */
    char buffer[findServerParams->bufLen];
    sockaddr_in client_addr;
    socklen_t client_addrlen;
    ssize_t num_bytes = -1;
    switch (status)
    {
        case -1:
            cerr << "poll() failed, " << status << endl;
            break;
        case 0:
            cerr << "Timeout of " << findServerParams->duration << " seconds expired." << endl;
            break;
        default:
            if (server_pollfds[0].revents & POLLIN)
            {
                num_bytes = recvfrom(server_socket,
                                     buffer,
                                     sizeof(buffer),
                                     0,
                                     reinterpret_cast<sockaddr *>(&client_addr),
                                     &client_addrlen);
            }
            else if (server_pollfds[0].revents & POLLPRI)
            {
                num_bytes = recvfrom(server_socket,
                                     buffer,
                                     sizeof(buffer),
                                     MSG_OOB,
                                     reinterpret_cast<sockaddr *>(&client_addr),
                                     &client_addrlen);
            }
            else if (server_pollfds[0].revents & POLLHUP)
            {
                cerr << "Remote server ended, continuing..." << endl;
            }
    }

    if (num_bytes > 0)
    {
        cout << "Data received from " << client_addr.sin_addr.s_addr << ":" << client_addr.sin_port << "<" << buffer
             << ">"
             << endl;

    }
    else if (num_bytes > 0)
    {
        /* Ignore */
    }
    else
    {
        cout << "No data received." << endl;
        cout << "Assuming server role." << endl;
    }
    close(server_socket);
    THREAD_EXIT(EXIT_SUCCESS);
}
