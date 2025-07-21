#include "CServer.hpp"
#include "HttpConnection.hpp"
#include "AsioIOServicePool.hpp"

CServer::CServer(net::io_context &ioc, unsigned short &port)
    : _ioc(ioc),_port(port),_acceptor(ioc, tcp::endpoint(tcp::v4(), port)) {
      std::cout << "Server start success, listen on port : " << _port << endl;
      startAccept();
    }

CServer::~CServer() {
  std::cout << "Server stop" << endl;
}

void CServer::startAccept() {
   auto &io_context = AsioIOServicePool::getInstance()->getIOService();
    shared_ptr<CSession> new_session = make_shared<CSession>(io_context, this);
    _acceptor.async_accept(new_session->getSocket(), std::bind(&CServer::handleAccept, this, new_session, placeholders::_1));
}

void CServer::clearSession(std::string sessionId) {
  std::lock_guard<std::mutex> lock(_mutex);
  if (_sessions.find(sessionId) != _sessions.end()) {
    _sessions.erase(sessionId);
  }
}


void CServer::handleAccept(shared_ptr<CSession>, const boost::system::error_code & error){

  if (!error) {
        new_session->start();
        lock_guard<mutex> lock(_mutex);
        _sessions.insert(make_pair(new_session->getUuid(), new_session));
    }
    else {
        Logger::log(LogLevel::error, "Accept error: " + error.message())
    }

    startAccept();
}