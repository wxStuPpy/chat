#include "VerifyGrpcClient.hpp"

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

std::unique_ptr<VerifyService::Stub> RPConPool::getConnection(){
    std::unique_lock<std::mutex> lock(_mutex);
    // 等待条件变量，直到有可用的连接或停止信号
    _cond.wait(lock, [this](){return !_connections.empty() || _isStop;});

    // 如果收到停止信号，则返回nullptr
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

 GetVerifyRsp VerifyGrpcClient::GetVerifyCode(std::string email) {
        ClientContext context;
        GetVerifyRsp reply;
        GetVerifyReq request;
        request.set_email(email);

        Status status = _stub->GetVerifyCode(&context, request, &reply);

        if (status.ok()) {

            return reply;
        }
        else {
            reply.set_error(ErrorCodes::RPCFailed);
            return reply;
        }
    }