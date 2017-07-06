#include <iostream>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include "Server.h"
#include "Client.h"

int
main()
{
//    boost::asio::io_service io_service;
    boost::thread server_thread {ManageServer, 2000, boost::chrono::seconds(5)};
    boost::thread client_thread {ManageClient};
    std::cout << __FUNCTION__ << boost::this_thread::get_id() << std::endl;
    client_thread.join();
    server_thread.join();
    return 0;
}
