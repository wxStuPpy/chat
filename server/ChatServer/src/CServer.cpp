#include "CServer.hpp"
#include "AsioIOServicePool.hpp"
#include "UserMgr.hpp"
using namespace std;
using namespace boost::asio;
using namespace boost::placeholders;
CServer::CServer(boost::asio::io_context& io_context, short port):_io_context(io_context), _port(port),
_acceptor(io_context, tcp::endpoint(tcp::v4(),port))
{
	cout << "Server start success, listen on port : " << _port << endl;
	startAccept();
}
CServer::~CServer() {
  std::cout << "Server stop" << endl;
}

void CServer::startAccept() {
   auto &io_context = AsioIOServicePool::getInstance()->getIOService();
    shared_ptr<CSession> new_session = make_shared<CSession>(io_context, this);
    _acceptor.async_accept(new_session->getSocket(), std::bind(&CServer::handleAccept, this, new_session,  std::placeholders::_1));
}

void CServer::clearSession(std::string sessionId) {

  if(_sessions.find(sessionId) != _sessions.end()){
    UserMgr::getInstance()->rmvUserSession(_sessions[sessionId]->getUserID());
  }
  {
    std::lock_guard<std::mutex> lock(_mutex); 
    _sessions.erase(sessionId);
  
}
}

void CServer::handleAccept(shared_ptr<CSession>new_session, const boost::system::error_code & error){

  if (!error) {
        new_session->start();
        lock_guard<mutex> lock(_mutex);
        _sessions.insert(make_pair(new_session->getSessionID(), new_session));
    }
    else {
        Logger::log(LogLevel::error, "Accept error: " + error.message());
    }
    startAccept();
}