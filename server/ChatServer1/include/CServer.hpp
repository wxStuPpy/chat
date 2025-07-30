#pragma once

#include "const.h"
#include "CSession.hpp"

class CServer
{
public:
    CServer(boost::asio::io_context& io_context, short port);
    ~CServer();
    void clearSession(std::string);
private:
    void handleAccept(shared_ptr<CSession>, const boost::system::error_code & error);
    void startAccept();
    boost::asio::io_context &_io_context;
    short _port;
    tcp::acceptor _acceptor;
    std::map<std::string, shared_ptr<CSession>> _sessions;
    std::mutex _mutex;
};