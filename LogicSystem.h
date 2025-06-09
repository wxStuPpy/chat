#pragma once
#include "const.h"

class HttpConnecton;
using HttpHanlder=std::function<void(HttpConnecton)>;

class LogicSystem:public Singleton<LogicSystem>
{
public:
~LogicSystem();
bool handleGet(std::string,std::shared_ptr<HttpConnecton>);
void regGet(std::string,HttpHanlder);
// bool handlePost(std::string,std::shared_ptr<HttpConnecton>);
// void regPost(std::string,HttpHanlder);
    
private:
    LogicSystem();
    std::map<std::string,HttpHanlder> post_handlers;
    std::map<std::string,HttpHanlder> get_handlers;
};