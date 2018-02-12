#include <iostream>
#include <boost/thread.hpp>
#include "Client.h"

void ManageClient()
{
    std::cout << __FUNCTION__ << boost::this_thread::get_id() << std::endl;
}