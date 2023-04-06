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

extern bool verbose;

class DafnyVerifierServiceImpl : public DafnyVerifierService::Service
{
    int numWorkers;
    sem_t countSem;
    std::string dafnyBinaryPath;
    pthread_mutex_t logFileLock;
    FILE* logFile;
    std::string WriteToTmpFile(const CloneAndVerifyRequest *request);
    std::string WriteToTmpFile(const VerificationRequest *request);
public:
    DafnyVerifierServiceImpl(int num_workers, std::string dafny_binary_path);
    virtual ~DafnyVerifierServiceImpl() {}

    Status Verify(ServerContext *context,
                  const VerificationRequest *request,
                  VerificationResponse *reply) override;

    Status CloneAndVerify(ServerContext *context,
                  const CloneAndVerifyRequest *request,
                  VerificationResponse *reply) override;

    Status CreateTmpFolder(ServerContext *context,
                  const CreateDir *request,
                  TmpFolder *reply) override;

    Status RemoveFolder(ServerContext *context,
                  const TmpFolder *request,
                  Empty *reply) override;

    Status DuplicateFolder(ServerContext *context,
                  const TmpFolder *request,
                  TmpFolder *reply) override;
};

#endif /* _VERIFIER_SERVICE_H_ */