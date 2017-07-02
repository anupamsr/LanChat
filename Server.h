#ifndef LANCHAT_SERVER_H
#define LANCHAT_SERVER_H

#include <unistd.h>
#include <netdb.h>

struct ManageServerParams
{
    char const *port;
    int duration;
    size_t buf_len;
};

void *
ManageServer(void *_manage_server_params);

#endif //LANCHAT_SERVER_H
