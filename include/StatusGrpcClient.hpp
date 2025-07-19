#pragma once
#include "const.h"
#include "Singleton.h"
#include "ConfigMgr.hpp"
#include "message.grpc.pb.h"
#include "message.pb.h"
#include <grpcpp/grpcpp.h>
#include <queue>
#include <condition_variable>
#include <atomic>

using grpc::Channel;
using grpc::Status;
using grpc::ClientContext;

using message::GetChatServerReq;
using message::GetChatServerRsp;
using message::StatusService;

class StatusConPool {
public:
    StatusConPool(size_t poolSize, std::string host, std::string port);
    ~StatusConPool() ;
    std::unique_ptr<StatusService::Stub> getConnection();
    void returnConnection(std::unique_ptr<StatusService::Stub> context) ;
    void close();
        
private:
    size_t _poolSize;
    std::string _host;
    std::string _port;
    std::atomic<bool> _isStop;
    std::queue<std::unique_ptr<StatusService::Stub>> _connections;
    std::mutex _mutex;
    std::condition_variable _cond;
};

class StatusGrpcClient :public Singleton<StatusGrpcClient>
{
    friend class Singleton<StatusGrpcClient>;
public:
    ~StatusGrpcClient() {
    }
    GetChatServerRsp GetChatServer(int uid);
    //LoginRsp Login(int uid, std::string token)
private:
    StatusGrpcClient();
    std::unique_ptr<StatusConPool> _pool;
};