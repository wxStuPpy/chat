#include "LogicSystem.hpp"
#include "HttpConnection.hpp"
#include "VerifyGrpcClient.hpp"
#include "RedisMgr.hpp"
#include "MysqlMgr.hpp"

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


 regPost("/user_register", [](std::shared_ptr<HttpConnection> conn) {
    auto body_str = boost::beast::buffers_to_string(conn->_request.body().data());
    std::cout << "receive body is " << body_str << std::endl;
    conn->_response.set(http::field::content_type, "text/json");
    json js = json::parse(body_str);
    json root;
    if (js.is_discarded()) {
        std::cout << "Failed to parse JSON data!" << std::endl;
        root["error"] = ErrorCodes::Error_Json;
        std::string jsonstr = root.dump();
        beast::ostream(conn->_response.body()) << jsonstr;
        return true;
    }
    auto email = js["email"].get<std::string>();
    auto name = js["user"].get<std::string>();
    auto pwd = js["passwd"].get<std::string>();
    auto confirm = js["confirm"].get<std::string>();
    if (pwd != confirm) {
        std::cout << "password err " << std::endl;
        root["error"] = ErrorCodes::PasswdErr;
        std::string jsonstr = root.dump();
        beast::ostream(conn->_response.body()) << jsonstr;
        return true;
    }
    //先查找redis中email对应的验证码是否合理
    std::string  verify_code;
    bool b_get_verify = RedisMgr::getInstance()->Get(CODEPREFIX+js["email"].get<std::string>(), verify_code);
    if (!b_get_verify) {
        std::cout << " get verify code expired" << std::endl;
        root["error"] = ErrorCodes::VerifyExpired;
        std::string jsonstr = root.dump();
        beast::ostream(conn->_response.body()) << jsonstr;
        return true;
    }
    if (verify_code != js["verifycode"].get<std::string>()) {
        std::cout << " verify code error" << std::endl;
        root["error"] = ErrorCodes::VerifyCodeErr;
        std::string jsonstr = root.dump();
        beast::ostream(conn->_response.body()) << jsonstr;
        return true;
    }
    //查找数据库判断用户是否存在
    int uid = MysqlMgr::getInstance()->RegUser(name, email, pwd);
    if (uid == 0 || uid == -1) {
        std::cout << " user or email exist" << std::endl;
        root["error"] = ErrorCodes::UserExist;
        std::string jsonstr = root.dump();
        beast::ostream(conn->_response.body()) << jsonstr;
        return true;
    }
    root["error"] = 0;
    root["uid"] = uid;
    root["email"] = email;
    root ["user"]= name;
    root["passwd"] = pwd;
    root["confirm"] = confirm;
    root["verifycode"] = js["verifycode"].get<std::string>();
    std::string jsonstr = root.dump();
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


