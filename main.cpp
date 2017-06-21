#include <iostream>
#include <netdb.h>
#include <arpa/inet.h>
#include <cstring>
#include <unistd.h>
#include <sys/poll.h>

using namespace std;

char const *const SERVER_PORT = "2000";

size_t const MAX_BUF_LEN = 250;

int const TIMEOUT_DURATION = 5; /* Seconds */

int
main()
{
    addrinfo hints, *server_addrinfo;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    int status = getaddrinfo("localhost", SERVER_PORT, &hints, &server_addrinfo);
    if (status < 0)
    {
        cerr << "getaddrinfo() failed, " << gai_strerror(status) << endl;
        return EXIT_FAILURE;
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
        return EXIT_FAILURE;
    }

    /* Now poll for already existing server on the network */
    pollfd server_pollfds[1];
    memset(&server_pollfds[0], 0, sizeof(pollfd));
    server_pollfds[0].fd = server_socket;
    server_pollfds[0].events = POLLIN | POLLPRI | POLLHUP;
    status = poll(server_pollfds, sizeof(server_pollfds) / sizeof(server_pollfds[0]), TIMEOUT_DURATION * 1000);

    /* Read data from server, if available */
    char buffer[MAX_BUF_LEN];
    sockaddr client_addr;
    socklen_t client_addrlen;
    ssize_t num_bytes = -1;
    switch (status)
    {
        case -1:cerr << "poll() failed, " << status << endl;
            return EXIT_FAILURE;
        case 0:cerr << "No data received in " << TIMEOUT_DURATION << " seconds." << endl;
        default:
            if (server_pollfds[0].revents & POLLIN)
            {
                num_bytes = recvfrom(server_socket, buffer, sizeof(buffer), 0, &client_addr, &client_addrlen);
            }
            else if (server_pollfds[0].revents & POLLPRI)
            {
                num_bytes = recvfrom(server_socket, buffer, sizeof(buffer), MSG_OOB, &client_addr, &client_addrlen);
            }
            else if (server_pollfds[0].revents & POLLHUP)
            {
                cerr << "Remote server ended, continuing..." << endl;
            }
    }

    if (num_bytes > 0)
    {
        cout << "Data received: <" << buffer << ">" << endl;
    }
    else
    {
        cout << "No data received." << endl;
    }

    close(server_socket);
    return 0;
}
