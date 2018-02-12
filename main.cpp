#include <iostream>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include "Singleton.h"
#include "Server.h"
#include "Client.h"

int
main()
{
    boost::thread server_thread {ManageServer, 2000, boost::chrono::seconds(5)};
    boost::thread client_thread {ManageClient};
    std::cout << __FUNCTION__ << boost::this_thread::get_id() << std::endl;
    client_thread.join();
    server_thread.join();
    return 0;
}
