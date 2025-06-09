#pragma once

#include "const.h"
#include <string>

class CServer : public std::enable_shared_from_this<CServer> {
public:
  CServer(net::io_context &ioc, unsigned short &port);
  ~CServer();
  void start();

private:
  net::io_context &_ioc;
  tcp::acceptor _acceptor;
  tcp::socket _socket;
};