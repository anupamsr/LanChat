#include <pthread.h>
#include <iostream>
#include "FindServer.h"

using namespace std;

char const *const SERVER_PORT = "2000";

int
main()
{
    cout << "Search for server" << endl;
    FindServerParams findServerParams;
    findServerParams.port = SERVER_PORT;
    findServerParams.duration = 5; /* Seconds */
    findServerParams.bufLen = 250;
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
