#include <iostream>
#include <netdb.h>
#include <arpa/inet.h>
#include <cstring>
#include <unistd.h>

using namespace std;

char const * const SERVER_PORT = "2000";
size_t const MAX_BUF_LEN = 250;
int const TIMEOUT_DURATION = 1;

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
        timeval duration;
        duration.tv_sec = TIMEOUT_DURATION;
        duration.tv_usec = 0;
        if (setsockopt(server_socket, SOL_SOCKET, SO_RCVTIMEO, &duration, sizeof(duration)) < 0)
        {
            close(server_socket);
            server_socket = -1;
            cerr << "setsockout() failed to set SO_RCVTIMEO, continuing..." << endl;
            continue;
        }
        if (bind(server_socket, p->ai_addr, p->ai_addrlen) < 0)
        {
            close(server_socket);
            server_socket = -1;
            cerr << "bind() failed, continuing..." << endl;
            continue;
        }
        sockaddr_in * server_sockaddr = reinterpret_cast< sockaddr_in *> (p->ai_addr);
        char server_name[INET_ADDRSTRLEN];
        inet_ntop(p->ai_family, &(server_sockaddr->sin_addr), server_name, INET_ADDRSTRLEN);
        cout << "Bound to socket at " << server_name << ":" << ntohs(server_sockaddr->sin_port) << endl;
        break;
    }

    freeaddrinfo(server_addrinfo);
    if (server_socket == -1)
    {
        cerr << "bind() failed" << endl;
    }

    char buffer[MAX_BUF_LEN];
    sockaddr client_addr;
    socklen_t client_addrlen;
    ssize_t num_bytes = recvfrom(server_socket, buffer, sizeof(buffer), 0, &client_addr, &client_addrlen);
    if (num_bytes < 0)
    {
        int err_no = errno;
        if (err_no == EAGAIN || errno == EWOULDBLOCK)
        {
            cerr << "No data recieved in " << TIMEOUT_DURATION << " seconds." << endl;
        }
        else
        {
            cerr << "recvfrom() failed." << endl;
        }
    }

    close(server_socket);
    return 0;
}