#include "CServer.hpp"
#include "HttpConnection.hpp"

CServer::CServer(net::io_context &ioc, unsigned short &port)
    : _ioc(ioc), _acceptor(ioc, tcp::endpoint(tcp::v4(), port)), _socket(ioc) {}

CServer::~CServer() {}

void CServer::start() {
  auto self(shared_from_this());
  _acceptor.async_accept(_socket, [self](boost::system::error_code ec) {
    try { // 如果有错误发生，则放弃这个连接，继续等待下一个连接。
      if (ec) {
        self->start();
        return;
      }
      //创建新连接 并创建HttpConnection对象，并开始处理请求。
      std::make_shared<HttpConnection>(std::move(_socket))->start();
      //继续监听
      self->start();
      
    } catch (const std::exception &e) {
      std::cerr << e.what() << '\n';
      self->start();
    }
  });
}