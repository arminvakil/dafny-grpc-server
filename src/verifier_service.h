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
    sem_t waitingOnGlobalMutexSem;
    std::string dafnyBinaryPath;
    pthread_mutex_t logFileLock;
    pthread_mutex_t globalLock;
    pthread_mutex_t coreListLock;
    FILE* logFile;
    std::string WriteToTmpFile(const VerificationRequest *request);
    std::queue<int> coreList;
public:
    DafnyVerifierServiceImpl(int num_workers, std::string dafny_binary_path);
    virtual ~DafnyVerifierServiceImpl() {}

    void LockGlobalMutex();
    void UnlockGlobalMutex();

    int AcquireCountSem(bool globalLockAlreadyAcquired);
    void ReleaseCountSem(int coreId);

    Status VerifySingleRequest(std::string requestId,
                               std::string codePath,
                               bool globalLockAlreadyAcquired,
                               const VerificationRequest *request,
                               VerificationResponse *reply);

    Status Verify(ServerContext *context,
                  const VerificationRequest *request,
                  VerificationResponse *reply) override;

    Status CloneAndVerify(ServerContext *context,
                  const CloneAndVerifyRequest *request,
                  VerificationResponseList *reply) override;

    Status TwoStageVerify(ServerContext *context,
                          const TwoStageRequest *request,
                          VerificationResponseList *reply) override;

    Status CreateTmpFolder(ServerContext *context,
                  const CreateDir *request,
                  TmpFolder *reply) override;

    Status RemoveFolder(ServerContext *context,
                  const TmpFolder *request,
                  Empty *reply) override;

    Status DuplicateFolder(ServerContext *context,
                  const TmpFolder *request,
                  TmpFolder *reply) override;

    Status WriteToRemoteFile(ServerContext *context,
                   const VerificationRequest *request,
                   Empty *reply) override;
};

#endif /* _VERIFIER_SERVICE_H_ */