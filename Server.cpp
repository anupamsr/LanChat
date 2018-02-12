#include <iostream>
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include "Server.h"
#include "Singleton.h"

void
ManageServer(const int _port, const boost::chrono::seconds &_interval)
{
    std::cout << __FUNCTION__ << boost::this_thread::get_id() << std::endl;
//    boost::this_thread::sleep_for(_interval);
    NetworkHandler::Server server(_port);
    std::cout << "Port: " << _port << std::endl;
}

NetworkHandler::Server::Server(const int _port)
    : port(_port),
      m_udp_socket(Singleton<boost::asio::io_service>::getInstance()),
      m_tcp_socket(Singleton<boost::asio::io_service>::getInstance())
{
    boost::asio::ip::udp::resolver::query query_local{"localhost", std::to_string(port)};
    boost::asio::ip::udp::resolver resolver(Singleton<boost::asio::io_service>::getInstance());
    std::string local_host = resolver.resolve(query_local)->host_name();

    boost::asio::ip::udp::resolver::query query(boost::asio::ip::udp::v4(), )
}

NetworkHandler::Server::~Server()
{
}

bool
NetworkHandler::Server::SearchServer()
{
    return false;
}

bool
NetworkHandler::Server::ConnectToServer()
{
    return false;
}

bool
NetworkHandler::Server::GetServerConnection()
{
    return false;
}

std::vector<std::string>
NetworkHandler::Server::RequestClientList()
{
    return std::vector<std::string>();
}

bool
NetworkHandler::Server::SendClientList(const std::vector<std::string> &_client_list)
{
    return false;
}

bool
NetworkHandler::Server::BecomeServer()
{
    return false;
}

bool
NetworkHandler::Server::AcceptClient()
{
    return false;
}

bool
NetworkHandler::Server::Shutdown()
{
    return false;
}

