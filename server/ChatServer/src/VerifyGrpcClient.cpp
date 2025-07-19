#include "VerifyGrpcClient.hpp"
#include "ConfigMgr.hpp"
#include "Logger.hpp"

RPConPool::RPConPool(size_t poolSize,std::string host,std::string port){
    for(size_t i = 0;i < poolSize;++i){
        std::unique_ptr<VerifyService::Stub> stub(VerifyService::NewStub(grpc::CreateChannel(host + ":" + port, grpc::InsecureChannelCredentials())));
        _connections.push(std::move(stub));
    }
}

RPConPool::~RPConPool(){
    std::lock_guard<std::mutex> lock(_mutex);
    close();
    while(!_connections.empty()){
        _connections.pop();
    }
}

void RPConPool::close(){
    _isStop = true;
    _cond.notify_all();
}

void RPConPool::retrunConnection(std::unique_ptr<VerifyService::Stub> stub){
    std::lock_guard<std::mutex> lock(_mutex);
    if(_isStop){
        return;
    }
    // 将连接放回队列中
    _connections.push(std::move(stub));
    _cond.notify_one();
}

std::unique_ptr<VerifyService::Stub> RPConPool::getConnection(){
    std::unique_lock<std::mutex> lock(_mutex);
    // 等待条件变量，直到有可用的连接或停止信号
    _cond.wait(lock, [this](){return !_connections.empty() || _isStop;});
    if(_isStop){
        return nullptr;
    }
    // 从连接队列中取出一个连接
    std::unique_ptr<VerifyService::Stub> stub = std::move(_connections.front());
    // 从队列中移除该连接
    _connections.pop();
    // 返回连接
    return stub;
}

VerifyGrpcClient::VerifyGrpcClient(){
    auto& gCfgMgr = ConfigMgr::getInstance();
    std::string host = gCfgMgr["VerifyServer"]["Host"];
    std::string port = gCfgMgr["VerifyServer"]["Port"];
    _pool.reset(new RPConPool(5, host, port));
}

 GetVerifyRsp VerifyGrpcClient::GetVerifyCode(std::string email) {
        ClientContext context;
        GetVerifyRsp reply;
        GetVerifyReq request;
        request.set_email(email);

        auto stub = _pool->getConnection();
        Status status = stub->GetVerifyCode(&context, request, &reply);

        if (status.ok()) {
            Logger::log(LogLevel::info, "GetVerifyCode RPC succeeded for email: " + email);
            _pool->retrunConnection(std::move(stub));
            return reply;
        }
        else {
            Logger::log(LogLevel::error, 
            "GetVerifyCode RPC failed for email: " + email +
            ", error code: " + std::to_string(status.error_code()) +
            ", error message: " + status.error_message());
            _pool->retrunConnection(std::move(stub));
            reply.set_error(ErrorCodes::RPCFailed);
            return reply;
        }
       
    }