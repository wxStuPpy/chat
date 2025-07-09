#include <grpcpp/grpcpp.h>
#include "message.grpc.pb.h"
#include "const.h"
#include "Singleton.h"

using grpc::Channel;
using grpc::Status;
using grpc::ClientContext;

using message::GetVerifyReq;
using message::GetVerifyRsp;
using message::VerifyService;

class RPConPool {
public:
    RPConPool(size_t poolSize,std::string host,std::string port);
    ~RPConPool();
    void close();
    std::unique_ptr<VerifyService::Stub>getConnection();
private:
  std::atomic<bool> _isStop;
  size_t _poolSize;
  std::string _host;
  std::string _port;
  std::queue<std::unique_ptr<VerifyService::Stub>> _connections;
  std::mutex _mutex;
  std::condition_variable _cond;
};

class VerifyGrpcClient:public Singleton<VerifyGrpcClient>
{
    friend class Singleton<VerifyGrpcClient>;
public:
    GetVerifyRsp GetVerifyCode(std::string email);

private:
    VerifyGrpcClient()=default;
    std::unique_ptr<VerifyService::Stub> _stub;
};