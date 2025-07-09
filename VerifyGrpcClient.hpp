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

class VerifyGrpcClient:public Singleton<VerifyGrpcClient>
{
    friend class Singleton<VerifyGrpcClient>;
public:

    GetVerifyRsp GetVerifyCode(std::string email) {
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

private:
    VerifyGrpcClient() {
        std::shared_ptr<Channel> channel = grpc::CreateChannel("127.0.0.1:50051", grpc::InsecureChannelCredentials());
        _stub = VerifyService::NewStub(channel);
    }

    std::unique_ptr<VerifyService::Stub> _stub;
};