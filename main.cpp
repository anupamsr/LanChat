#include <pthread.h>
#include <iostream>
#include "Globals.h"
#include "FindServer.h"

using namespace std;

pthread_mutex_t client_list_m;

int
main()
{
    cout << "Search for server" << endl;
    FindServerParams findServerParams;
    findServerParams.port = SERVER_PORT;
    findServerParams.duration = CONN_TIMEOUT;
    findServerParams.bufLen = MAX_BUF_LEN;
    pthread_mutex_init(&client_list_m, NULL);
    pthread_t server_thread;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    int status = pthread_create(
        &server_thread,
        &attr,
        FindServer,
        &findServerParams);
    pthread_attr_destroy(&attr);
    if (status != 0)
    {
        cerr << "ERROR: Could not create thread, exiting..." << std::endl;
        return status;
    }

    pthread_join(server_thread, NULL);

    string chat_text;
    cout << "On-line..." << endl;
    cout << "You: ";
    cin >> chat_text;

    pthread_exit(NULL);
}
