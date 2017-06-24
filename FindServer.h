#ifndef LANCHAT_FINDSERVER_H
#define LANCHAT_FINDSERVER_H

#include <unistd.h>
#include <netdb.h>

struct FindServerParams
{
    char const *port;
    int duration;
    size_t bufLen;
};

void *
FindServer(void *_findServerParams);

#endif //LANCHAT_FINDSERVER_H
