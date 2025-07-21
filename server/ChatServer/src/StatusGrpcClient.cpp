#include "StatusGrpcClient.hpp"

StatusConPool::StatusConPool(size_t poolSize, std::string host, std::string port)
    : _poolSize(poolSize), _host(host), _port(port), _isStop(false) {
    for (size_t i = 0; i < _poolSize; ++i) {
        std::shared_ptr<Channel> channel = grpc::CreateChannel(_host + ":" + _port,
             grpc::InsecureChannelCredentials());
        _connections.push(StatusService::NewStub(channel));
    }
}

StatusConPool::~StatusConPool() {
    std::lock_guard<std::mutex> lock(_mutex);
    close();
    while (!_connections.empty()) {
        _connections.pop();
    }
}

std::unique_ptr<StatusService::Stub> StatusConPool::getConnection() {
    std::unique_lock<std::mutex> lock(_mutex);
     _cond.wait(lock, [this] {
        if (_isStop) {
            return true;
        }
        return !_connections.empty();
        });
        if (_isStop) {
            return nullptr;
        }
        auto context = std::move(_connections.front());
        _connections.pop();
        return context;
}

 void StatusConPool::returnConnection(std::unique_ptr<StatusService::Stub> context) {
    std::lock_guard<std::mutex> lock(_mutex);
    if (_isStop) {
        return;
    }
    _connections.push(std::move(context));
    _cond.notify_one();
}

void StatusConPool::close() {
    _isStop = true;
    _cond.notify_all();
}

GetChatServerRsp StatusGrpcClient::GetChatServer(int uid)
{
    ClientContext context;
    GetChatServerRsp reply;
    GetChatServerReq request;
    request.set_uid(uid);
    auto stub = _pool->getConnection();
    Status status = stub->GetChatServer(&context, request, &reply);
    Defer defer([&stub, this]() {
        _pool->returnConnection(std::move(stub));
        });
    if (status.ok()) {    
        return reply;
    }
    else {
        reply.set_error(ErrorCodes::RPCFailed);
        return reply;
    }
}
StatusGrpcClient::StatusGrpcClient()
{
    auto& gCfgMgr = ConfigMgr::getInstance();
    std::string host = gCfgMgr["StatusServer"]["Host"];
    std::string port = gCfgMgr["StatusServer"]["Port"];
    _pool.reset(new StatusConPool(5, host, port));
}
