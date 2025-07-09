#include "LogicSystem.hpp"
#include "HttpConnection.hpp"
#include "VerifyGrpcClient.hpp"

LogicSystem::LogicSystem() {
  regGet("/get_test", [](std::shared_ptr<HttpConnection> conn) {
    beast::ostream(conn->_response.body()) << "recvive get_test request";
    int i = 0;
    for (auto &elem : conn->_getParams) {
      i++;
      beast::ostream(conn->_response.body())
          << "param" << i << " key is " << elem.first;
      beast::ostream(conn->_response.body())
          << ", " << " value is " << elem.second << std::endl;
    }
  });
  regPost("/post_test", [](std::shared_ptr<HttpConnection> conn) {
    auto body_str =
        boost::beast::buffers_to_string(conn->_request.body().data());
    std::cout << "body is :" << body_str << std::endl;
    conn->_response.set(http::field::content_type, "text/json");
    json js = json::parse(body_str);
    json root;
    if (js.is_discarded()) { // 检查是否解析失败
      std::cerr << "failed to parse JSON" << std::endl;
      root["error"] = ErrorCodes::Error_Json;
      std::string jsonstr=root.dump();
      beast::ostream(conn->_response.body()) << jsonstr;
      return true;
    } 

    if(!js.contains("email")){
      std::cerr << "failed to parse JSON EMAIL" << std::endl;
      root["error"] = ErrorCodes::Error_Json;
      std::string jsonstr=root.dump();
      beast::ostream(conn->_response.body()) << jsonstr;
      return true;
    }

    auto email=js["email"].get<std::string>();
    std::cout << "email is :" << email << std::endl;
    GetVerifyRsp rsp = VerifyGrpcClient::getInstance()->GetVerifyCode(email);
    root["email"]=js["email"];
    root["error"] = rsp.error();;
    std::string jsonstr=root.dump();
    beast::ostream(conn->_response.body()) << jsonstr;
    return true;
  });
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


