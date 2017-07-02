#include <pthread.h>
#include <iostream>
#include <algorithm>
#include "log4cxx/basicconfigurator.h"
#include "log4cxx/helpers/exception.h"
#include "Globals.h"
#include "Connect.h"
#include "INIFile.h"
#include "Server.h"
#include "Singleton.h"

using namespace std;

pthread_mutex_t client_list_m;

int
main()
{
    int result = EXIT_SUCCESS;
    try
    {
        // Set up a simple configuration that logs on the console.
        log4cxx::BasicConfigurator::configure();
        pthread_mutex_init(&client_list_m, NULL);

        LOG4CXX_INFO(LanChatLogger::getLogger(), "Reading INI file")

        INIFile ini_file;
        vector<Client> client_list = ini_file.Read();
        pthread_mutex_lock(&client_list_m);
        Singleton<ClientMap>::getInstance().clear();
        for (auto it = client_list.begin(); it != client_list.end(); ++it)
        {
            Singleton<ClientMap>::getInstance().insert(std::pair<std::string, Client>(it->address, *it));
        }

        pthread_mutex_unlock(&client_list_m);

        LOG4CXX_INFO(LanChatLogger::getLogger(), "Search for server");

        ManageServerParams manage_server_params;
        manage_server_params.port = PORT;
        manage_server_params.duration = CONN_TIMEOUT;
        manage_server_params.buf_len = MAX_BUF_LEN;

        pthread_t server_thread;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
        int status = pthread_create(&server_thread, &attr, ManageServer, &manage_server_params);
        pthread_attr_destroy(&attr);
        if (status != 0)
        {
            LOG4CXX_FATAL(LanChatLogger::getLogger(), "Could not create thread, exiting...");
            return EXIT_FAILURE;
        }

        StartClient();

        pthread_join(server_thread, NULL);
        LOG4CXX_INFO(LanChatLogger::getLogger(), "Exiting application.")
    }
    catch (log4cxx::helpers::Exception &)
    {
        result = EXIT_FAILURE;
    }

    return result;
}
