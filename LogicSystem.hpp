#pragma once
#include "const.h"

class HttpConnection;
using HttpHandler = std::function<void(std::shared_ptr<HttpConnection>)>;

class LogicSystem : public Singleton<LogicSystem>
{   
    friend class Singleton<LogicSystem>;
public:
    bool handleGet(std::string, std::shared_ptr<HttpConnection>);
    void regGet(std::string, HttpHandler);
    bool handlePost(std::string, std::shared_ptr<HttpConnection>);
    void regPost(std::string, HttpHandler);
    // bool handlePost(std::string,std::shared_ptr<HttpConnection>);
    // void regPost(std::string,HttpHandler);

private:
    LogicSystem();
    std::map<std::string, HttpHandler> _postHandlers;
    std::map<std::string, HttpHandler> _getHandlers;
};