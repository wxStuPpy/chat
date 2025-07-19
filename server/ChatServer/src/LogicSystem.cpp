#include "LogicSystem.hpp"
#include "HttpConnection.hpp"
#include "VerifyGrpcClient.hpp"
#include "StatusGrpcClient.hpp"
#include "RedisMgr.hpp"
#include "MysqlMgr.hpp"
#include "Logger.hpp"

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
  regPost("/get_verifycode", [](std::shared_ptr<HttpConnection> conn) {
    auto body_str =
        boost::beast::buffers_to_string(conn->_request.body().data());
    Logger::log(LogLevel::info, "receive body is "+ body_str);
    conn->_response.set(http::field::content_type, "text/json");
    json js = json::parse(body_str);
    json root;
    if (js.is_discarded()) { // 检查是否解析失败
      Logger::log(LogLevel::error, "failed to parse JSON");
      root["error"] = ErrorCodes::Error_Json;
      std::string jsonstr=root.dump();
      beast::ostream(conn->_response.body()) << jsonstr;
      return true;
    } 

    if(!js.contains("email")){
      Logger::log(LogLevel::error, "json has no email");
      root["error"] = ErrorCodes::Error_Json;
      std::string jsonstr=root.dump();
      beast::ostream(conn->_response.body()) << jsonstr;
      return true;
    }

    auto email=js["email"].get<std::string>();
    Logger::log(LogLevel::info, "email is "+ email);
    GetVerifyRsp rsp = VerifyGrpcClient::getInstance()->GetVerifyCode(email);
    root["email"]=js["email"];
    root["error"] = rsp.error();;
    std::string jsonstr=root.dump();
    beast::ostream(conn->_response.body()) << jsonstr;
    return true;
  });


 regPost("/user_register", [](std::shared_ptr<HttpConnection> conn) {
    auto body_str = boost::beast::buffers_to_string(conn->_request.body().data());
    Logger::log(LogLevel::info, "register receive body is "+body_str);
    conn->_response.set(http::field::content_type, "text/json");
    json js = json::parse(body_str);
    json root;
    if (js.is_discarded()) {
        Logger::log(LogLevel::error, "failed to parse JSON");
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
       Logger::log(LogLevel::error, "passwd not match");
        root["error"] = ErrorCodes::PasswdErr;
        std::string jsonstr = root.dump();
        beast::ostream(conn->_response.body()) << jsonstr;
        return true;
    }
    //先查找redis中email对应的验证码是否合理
    std::string  verify_code;
    bool b_get_verify = RedisMgr::getInstance()->Get(CODEPREFIX+js["email"].get<std::string>(), verify_code);
    if (!b_get_verify) {
        Logger::log(LogLevel::error, " get verify code expired");
        root["error"] = ErrorCodes::VerifyExpired;
        std::string jsonstr = root.dump();
        beast::ostream(conn->_response.body()) << jsonstr;
        return true;
    }
    if (verify_code != js["verifycode"].get<std::string>()) {
        Logger::log(LogLevel::error, " verify code error");
        root["error"] = ErrorCodes::VerifyCodeErr;
        std::string jsonstr = root.dump();
        beast::ostream(conn->_response.body()) << jsonstr;
        return true;
    }
    //查找数据库判断用户是否存在
    int uid = MysqlMgr::getInstance()->RegUser(name, email, pwd);
    if (uid == 0 || uid == -1) {
        Logger::log(LogLevel::error, " user exist");
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

    //重置回调逻辑
    regPost("/reset_pwd", [](std::shared_ptr<HttpConnection> conn) {
    auto body_str = boost::beast::buffers_to_string(conn->_request.body().data());
    Logger::log(LogLevel::info, "reset receive body is "+ body_str);
    conn->_response.set(http::field::content_type, "text/json");
    json js = json::parse(body_str);
    json root;
    if (js.is_discarded()) {
        Logger::log(LogLevel::error, "failed to parse JSON");
        root["error"] = ErrorCodes::Error_Json;
        std::string jsonstr = root.dump();
        beast::ostream(conn->_response.body()) << jsonstr;
        return true;
    }
    auto email = js["email"].get<std::string>();
    auto name = js["user"].get<std::string>();
    auto pwd = js["passwd"].get<std::string>();
    //先查找redis中email对应的验证码是否合理
    std::string  verify_code;
    bool b_get_verify = RedisMgr::getInstance()->Get(CODEPREFIX + js["email"].get<std::string>(), verify_code);
    if (!b_get_verify) {
        Logger::log(LogLevel::error, " get verify code expired");
        root["error"] = ErrorCodes::VerifyExpired;
        std::string jsonstr = root.dump();
        beast::ostream(conn->_response.body()) << jsonstr;
        return true;
    }
    if (verify_code != js["verifycode"].get<std::string>()) {
        Logger::log(LogLevel::error, " verify code error");
        root["error"] = ErrorCodes::VerifyCodeErr;
        std::string jsonstr = root.dump();
        beast::ostream(conn->_response.body()) << jsonstr;
        return true;
    }
    //查询数据库判断用户名和邮箱是否匹配
    bool email_valid = MysqlMgr::getInstance()->CheckEmail(name, email);
    if (!email_valid) {
        Logger::log(LogLevel::error, " email not match");
        root["error"] = ErrorCodes::EmailNotMatch;
        std::string jsonstr = root.dump();
        beast::ostream(conn->_response.body()) << jsonstr;
        return true;
    }
    //更新密码为最新密码
    bool b_up = MysqlMgr::getInstance()->UpdatePwd(name, pwd);
    if (!b_up) {
       Logger::log(LogLevel::error, " update password failed");
        root["error"] = ErrorCodes::PasswdUpFailed;
        std::string jsonstr = root.dump();
        beast::ostream(conn->_response.body()) << jsonstr;
        return true;
    }
    Logger::log(LogLevel::info, " reset password succeed to "+ pwd);
    root["error"] = 0;
    root["email"] = email;
    root["user"] = name;
    root["passwd"] = pwd;
    root["verifycode"] = js["verifycode"].get<std::string>();
    std::string jsonstr = root.dump();
    beast::ostream(conn->_response.body()) << jsonstr;
    return true;
    });

    regPost("/user_login", [](std::shared_ptr<HttpConnection> conn) {
    auto body_str = boost::beast::buffers_to_string(conn->_request.body().data());
    Logger::log(LogLevel::info, "login receive body is "+ body_str);
    conn->_response.set(http::field::content_type, "text/json");
    json js=json::parse(body_str);
    json root;
    if (js.is_discarded()) {
        Logger::log(LogLevel::error, "failed to parse JSON");
        root["error"] = ErrorCodes::Error_Json;
        std::string jsonstr = root.dump();
        beast::ostream(conn->_response.body()) << jsonstr;
        return true;
    }
    auto name = js["user"].get<std::string>();
    auto pwd = js["passwd"].get<std::string>();
    UserInfo userInfo;
    //查询数据库判断用户名和密码是否匹配
    bool pwd_valid = MysqlMgr::getInstance()->CheckPwd(name, pwd, userInfo);
    if (!pwd_valid) {
        Logger::log(LogLevel::error, " password invalid");
        root["error"] = ErrorCodes::PasswdInvalid;
        std::string jsonstr = root.dump();
        beast::ostream(conn->_response.body()) << jsonstr;
        return true;
    }
    //查询StatusServer找到合适的连接
    auto reply = StatusGrpcClient::getInstance()->GetChatServer(userInfo._uid);
    if (reply.error()) {
        Logger::log(LogLevel::error, " rpc get chat server failed"+ reply.error());
        root["error"] = ErrorCodes::RPCGetFailed;
        std::string jsonstr = root.dump();
        beast::ostream(conn->_response.body()) << jsonstr;
        return true;
    }
    Logger::log(LogLevel::info, " login succeed to "+ name);
    root["error"] = 0;
    root["user"] = name;
    root["uid"] = userInfo._uid;
    root["token"] = reply.token();
    root["host"] = reply.host();
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
    Logger::log(LogLevel::error,"get handler not found");
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
    Logger::log(LogLevel::error,"post handler not found");
    return false;
}
void LogicSystem::regPost(std::string url, HttpHandler handler){
    _postHandlers[url]=handler;
}


