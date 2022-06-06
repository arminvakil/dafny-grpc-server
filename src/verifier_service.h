#ifndef _VERIFIER_SERVICE_H_
#define _VERIFIER_SERVICE_H_

#include <fstream>
#include <ios>
#include <mutex>
#include <queue>
#include <semaphore.h>
#include <unordered_map>
#include <unordered_set>

#include <google/protobuf/util/delimited_message_util.h>
#include <grpcpp/grpcpp.h>
#include "protos/verifier.pb.h"
#include "protos/verifier.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using namespace DafnyExecutorServer;

class DafnyVerifierServiceImpl : public DafnyVerifierService::Service
{
    int numWorkers;
    sem_t countSem;
    // FILE* logFile;
    // pthread_mutex_t logFileLock;
public:
    DafnyVerifierServiceImpl(int num_workers);
    virtual ~DafnyVerifierServiceImpl() {}

    Status Verify(ServerContext *context,
                  const VerificationRequest *request,
                  VerificationResponse *reply) override;
};

#endif /* _VERIFIER_SERVICE_H_ */