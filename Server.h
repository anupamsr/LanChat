#ifndef LANCHAT_MANAGESERVER_H
#define LANCHAT_MANAGESERVER_H

#include <vector>
#include <boost/asio.hpp>

namespace NetworkHandler
{
class Server
{
public:
    Server(const int _port);
    virtual ~Server();
    virtual bool
    SearchServer();
    virtual bool
    ConnectToServer();
    virtual bool
    GetServerConnection();
    virtual std::vector<std::string>
    RequestClientList();
    virtual bool
    SendClientList(const std::vector<std::string> &_client_list);
    virtual bool
    BecomeServer();
    virtual bool
    AcceptClient();
    virtual bool
    Shutdown();

private:
    int port;
    boost::asio::ip::udp::socket m_udp_socket;
    boost::asio::ip::tcp::socket m_tcp_socket;
};
}
void
ManageServer(const int _port, const boost::chrono::seconds &_interval);


#endif //LANCHAT_MANAGESERVER_H
