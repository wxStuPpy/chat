#include "LogicSystem.hpp"
#include "HttpConnection.hpp"

bool LogicSystem::handleGet(std::string path ,std::shared_ptr<HttpConnection>conn){
    if(_getHandlers.find(path) != _getHandlers.end()){
        _getHandlers[path](conn);
        return true;
    }
    return false;
}

void LogicSystem::regGet(std::string url,HttpHandler handler){
    _getHandlers[url] = handler;
}

LogicSystem::LogicSystem(){
    regGet("/get_test",[](std::shared_ptr<HttpConnection> conn){
        beast::ostream(conn->_response.body()) << "recvive get_test request";
      });
}

