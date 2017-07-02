#ifndef LANCHAT_GLOBALS_H
#define LANCHAT_GLOBALS_H

#include <pthread.h>
#include <string>
#include <map>
#include "Client.h"

#define THREAD_EXIT(retVal) return reinterpret_cast<void *>(retVal)

#define SERVER Singleton<Client>::getInstance()

char const *const INI_FILE = "LanChat.ini";

char const *const PORT = "2000";

const int MAX_BUF_LEN = 250;

const int CONN_TIMEOUT = 5; /* Seconds */

char const *const SERVER_HEADER = "SERVER LanChat v1";

char const *const CLIENT_HEADER = "CLIENT LanChat v1";

char const *const SERVER_REPLY = "SERVER LanChat client added";

char const *const PUT_CLIENT_LIST = "LanChat v1 PUT CLIENT LIST";

char const *const GET_CLIENT_LIST = "LanChat v1 GET CLIENT LIST";

char const *const SERVER_CLOSING = "SERVER LanChat v1 Closing";

extern pthread_mutex_t client_list_m;

extern pthread_mutex_t ini_file_m;

typedef std::map<std::string, Client> ClientMap;

#endif //LANCHAT_GLOBALS_H
