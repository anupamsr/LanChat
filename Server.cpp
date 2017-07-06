#include <iostream>
#include <boost/thread.hpp>
#include "Server.h"

void ManageServer(const int _port, const boost::chrono::seconds & _interval)
{
    std::cout << __FUNCTION__ << boost::this_thread::get_id() << std::endl;
    boost::this_thread::sleep_for(_interval);
    std::cout << "Port: " << _port << std::endl;
}