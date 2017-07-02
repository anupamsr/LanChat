#ifndef LANCHAT_CONNECT_H
#define LANCHAT_CONNECT_H

#include <netdb.h>

int
BindToSocketUDP(const char *_port, addrinfo &_remote_addrinfo);

int
BindToSocketTCP(const char *_address, const char *_port, addrinfo &_remote_addrinfo);

#endif //LANCHAT_CONNECT_H
