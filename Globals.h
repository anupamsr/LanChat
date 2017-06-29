#ifndef LANCHAT_CONSTANTS_H_H
#define LANCHAT_CONSTANTS_H_H

#include <pthread.h>

#define THREAD_RETURN(retVal) reinterpret_cast<void *>(retVal);

char const *const SERVER_PORT = "2000";

const int MAX_BUF_LEN = 250;
const int CONN_TIMEOUT = 5; /* Seconds */

char const * const SERVER_HEADER = "SERVER LanChat v1";
char const * const CLIENT_HEADER = "CLIENT LanChat v1";
char const * const SERVER_REPLY = "SERVER LanChat client added";
char const * const PUT_CLIENT_LIST = "LanChat v1 PUT CLIENT LIST";
char const * const GET_CLIENT_LIST = "LanChat v1 GET CLIENT LIST";

extern pthread_mutex_t client_list_m;

#endif //LANCHAT_CONSTANTS_H_H
