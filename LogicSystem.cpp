#include "LogicSystem.hpp"
#include "HttpConnection.hpp"

LogicSystem::LogicSystem(){
    regGet("/get_test",[](std::shared_ptr<HttpConnection> conn){
        beast::ostream(conn->_response.body()) << "recvive get_test request";
        int i = 0;
        for (auto& elem : conn->_getParams) {
            i++;
            beast::ostream(conn->_response.body()) << "param" << i << " key is " << elem.first;
            beast::ostream(conn->_response.body()) << ", " <<  " value is " << elem.second << std::endl;
        }
      });
    //   regPost("/post_test/",[](std::shared_ptr<HttpConnection> conn){
    //     beast::ostream(conn->_response.body()) << "recvive post_test request";
    //   });
}


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

bool LogicSystem::handlePost(std::string path, std::shared_ptr<HttpConnection>conn){
    if(_postHandlers.find(path) != _postHandlers.end()){
        _postHandlers[path](conn);
        return true;
    }
    return false;
}
void LogicSystem::regPost(std::string url, HttpHandler handler){
    _postHandlers[url]=handler;
}


