#include <assert.h>
#include <sstream>
#include <sys/wait.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <cstdlib>
#include <sys/types.h>
#include <sys/stat.h>
#include <future>

#include <grpc++/grpc++.h>

#include "verifier_service.h"
#define BUFFER_SIZE 10000

using namespace std;
using namespace std::chrono;
using grpc::StatusCode;

#define LOG(...)                              \
    if (verbose)                              \
    {                                         \
        pthread_mutex_lock(&logFileLock);     \
        logFile = fopen("/tmp/log.txt", "a"); \
        fprintf(logFile, __VA_ARGS__);        \
        fclose(logFile);                      \
        pthread_mutex_unlock(&logFileLock);   \
    }

DafnyVerifierServiceImpl::DafnyVerifierServiceImpl(int num_workers, string dafny_binary_path) : numWorkers(num_workers), dafnyBinaryPath(dafny_binary_path)
{
    sem_init(&countSem, 1, numWorkers);
    sem_init(&waitingOnGlobalMutexSem, 1, 0);
    pthread_mutex_init(&logFileLock, 0);
    pthread_mutex_init(&globalLock, 0);
    pthread_mutex_init(&coreListLock, 0);
    for (int i = 0; i < numWorkers; i++) {
        coreList.push(i);
    }
    logFile = fopen("/tmp/log.txt", "w");
    fclose(logFile);
}

int DafnyVerifierServiceImpl::AcquireCountSem(bool globalLockAlreadyAcquired) {
    if (!globalLockAlreadyAcquired) {
        int waitingOnGlobalMutexSemValue;
        sem_getvalue(&waitingOnGlobalMutexSem, &waitingOnGlobalMutexSemValue);
        // if a request is waiting on global sem, we should wait here
        while (waitingOnGlobalMutexSemValue != 0) {
            usleep(1000);
            sem_getvalue(&waitingOnGlobalMutexSem, &waitingOnGlobalMutexSemValue);
        }
    }
    sem_wait(&countSem);
    pthread_mutex_lock(&coreListLock);
    int coreId = coreList.front();
    coreList.pop();
    pthread_mutex_unlock(&coreListLock);
    return coreId;
}

void DafnyVerifierServiceImpl::ReleaseCountSem(int coreId) {
    pthread_mutex_lock(&coreListLock);
    coreList.push(coreId);
    pthread_mutex_unlock(&coreListLock);
    sem_post(&countSem);
}

void DafnyVerifierServiceImpl::LockGlobalMutex() {
    sem_post(&waitingOnGlobalMutexSem);
    int countSemValue;
    sem_getvalue(&countSem, &countSemValue);
    // wait until all singleton threads are finished
    while (countSemValue != numWorkers) {
        usleep(1000);
        sem_getvalue(&countSem, &countSemValue);
    }
    pthread_mutex_lock(&globalLock);
    sem_wait(&waitingOnGlobalMutexSem);
}

void DafnyVerifierServiceImpl::UnlockGlobalMutex() {
    pthread_mutex_unlock(&globalLock);
}

Status DafnyVerifierServiceImpl::CreateTmpFolder(ServerContext *context,
                                                 const CreateDir *request,
                                                 TmpFolder *reply)
{
    char tmplate[] = "/dev/shm/verifier_dir_XXXXXX";
    char *tmp_dir = mkdtemp(tmplate);
    reply->set_path(tmp_dir);
    if (request->owner() != "") {
        std::string chown_cmd = "sudo chown -R ";
        chown_cmd.append(request->owner());
        chown_cmd.append(" ");
        chown_cmd.append(tmp_dir);
        system(chown_cmd.c_str());
    }
    return Status::OK;
}

Status DafnyVerifierServiceImpl::RemoveFolder(ServerContext *context,
                                                 const TmpFolder *request,
                                                 Empty *reply)
{
    struct stat info;

    if (stat(request->path().c_str(), &info) != 0) {
        std::string msg("Removing folder does not exist! path = ");
        msg.append(request->path());
        return Status(StatusCode::INVALID_ARGUMENT, msg);
    }
    else if (!(info.st_mode & S_IFDIR)) {
        std::string msg("Removing path is not a folder! path = ");
        msg.append(request->path());
        return Status(StatusCode::INVALID_ARGUMENT, msg);
    }
    std::string rm_cmd = "rm -rf ";
    rm_cmd.append(request->path());
    system(rm_cmd.c_str());
    return Status::OK;
}

Status DafnyVerifierServiceImpl::DuplicateFolder(ServerContext *context,
                                                 const TmpFolder *request,
                                                 TmpFolder *reply)
{
    struct stat info;

    if (stat(request->path().c_str(), &info) != 0) {
        std::string msg("Duplicating folder does not exist! path = ");
        msg.append(request->path());
        return Status(StatusCode::INVALID_ARGUMENT, msg);
    }
    else if (!(info.st_mode & S_IFDIR)) {
        std::string msg("Duplicating folder is not a folder! path = ");
        msg.append(request->path());
        return Status(StatusCode::INVALID_ARGUMENT, msg);
        return Status::CANCELLED;
    }
    char tmplate[] = "/dev/shm/verifier_dir_XXXXXX";
    char *tmp_dir = mkdtemp(tmplate);
    std::string copy_cmd = "cp -r ";
    copy_cmd.append(request->path());
    copy_cmd.append("/* ");
    copy_cmd.append(tmp_dir);
    system(copy_cmd.c_str());
    if (request->owner() != "") {
        std::string chown_cmd = "sudo chown -R ";
        chown_cmd.append(request->owner());
        chown_cmd.append(" ");
        chown_cmd.append(tmp_dir);
        system(chown_cmd.c_str());
    }
    if (request->modifyingfile() != "") {
        std::string removing_file_path = tmp_dir;
        removing_file_path.append("/");
        removing_file_path.append(request->modifyingfile());
        // std::cerr << "removing :" << removing_file_path << "\n";
        if (stat(removing_file_path.c_str(), &info) != 0) {
            std::string msg("Removing file path does not exists! removing_file_path = ");
            msg.append(removing_file_path);
            return Status(StatusCode::INVALID_ARGUMENT, msg);
        }
        else {
            std::string rm_cmd = "rm ";
            rm_cmd.append(removing_file_path);
            system(rm_cmd.c_str());
        }
    }
    reply->set_path(tmp_dir);
    return Status::OK;
}

std::string DafnyVerifierServiceImpl::WriteToTmpFile(const VerificationRequest *request)
{
    std::string tmpFileName = std::tmpnam(nullptr);
    tmpFileName.append(".dfy");
    LOG("request creating a temporary file at %s\n", tmpFileName.c_str());
    FILE *file = fopen(tmpFileName.c_str(), "w");
    if (file == NULL)
    {
        perror("Error opening temporary file");
        return "";
    }
    fputs(request->code().c_str(), file);
    fclose(file);
    return tmpFileName;
}

Status WriteToFile(const VerificationRequest *request)
{
    FILE *file = fopen(request->path().c_str(), "w");
    if (file == NULL)
    {
        std::string msg = "Error opening file ";
        msg.append(request->path());
        perror(msg.c_str());
        return Status::CANCELLED;
    }
    fputs(request->code().c_str(), file);
    fclose(file);
    return Status::OK;
}

Status WriteToFile(const string &path, const string &code)
{
    FILE *file = fopen(path.c_str(), "w");
    if (file == NULL)
    {
        std::string msg = "Error opening file ";
        msg.append(path);
        perror(msg.c_str());
        return Status::CANCELLED;
    }
    fputs(code.c_str(), file);
    fclose(file);
    return Status::OK;
}

Status DafnyVerifierServiceImpl::WriteToRemoteFile(ServerContext *context,
                   const VerificationRequest *request,
                   Empty *reply)
{
    FILE *file = fopen(request->path().c_str(), "w");
    if (file == NULL)
    {
        std::string msg = "Error opening file ";
        msg.append(request->path());
        perror(msg.c_str());
        return Status::CANCELLED;
    }
    fputs(request->code().c_str(), file);
    fclose(file);
    return Status::OK;
}

Status DafnyVerifierServiceImpl::CloneAndVerify(ServerContext *context,
                                                const CloneAndVerifyRequest *request,
                                                VerificationResponseList *reply)
{
    struct stat info;
    std::string codePath;
    char *tmp_dir = NULL;
    std::string dir_to_delete = "";
    string requestId;
    auto start = time_point_cast<milliseconds>(system_clock::now());
    auto start_from_epoch = start.time_since_epoch().count();

    if (request->directorypath() != "") {
        if (stat(request->directorypath().c_str(), &info) != 0) {
            std::string msg("Duplicating folder does not exist! directorypath = ");
            msg.append(request->directorypath());
            return Status(StatusCode::INVALID_ARGUMENT, msg);
        }
        else if (!(info.st_mode & S_IFDIR)) {
            std::string msg("Duplicating folder is not a folder! directorypath = ");
            msg.append(request->directorypath());
            return Status(StatusCode::INVALID_ARGUMENT, msg);
            return Status::CANCELLED;
        }
        char tmplate[] = "/dev/shm/verifier_dir_XXXXXX";
        tmp_dir = strdup(mkdtemp(tmplate));
        std::string copy_cmd = "cp -r ";
        copy_cmd.append(request->directorypath());
        copy_cmd.append("/* ");
        copy_cmd.append(tmp_dir);
        dir_to_delete.append(tmp_dir);
        system(copy_cmd.c_str());
        codePath = tmp_dir;
        codePath += "/" + request->modifyingfile();
        requestId = request->directorypath();
        requestId.append(&(tmp_dir[9]));
        requestId.push_back('_');
    }
    else {
        // sem_post(&countSem);
        std::string msg("Duplicating folder not given! directorypath == \"\"");
        return Status(StatusCode::INVALID_ARGUMENT, msg);
    }

    if (request->modifyingfile() != "") {
        FILE *file = fopen(codePath.c_str(), "w");
        if (file == NULL)
        {
            // sem_post(&countSem);
            std::string msg = "Error opening file ";
            msg.append(codePath);
            perror(msg.c_str());
        
            return Status::CANCELLED;
        }
        fputs(request->code().c_str(), file);
        fclose(file);
    }

    for (int i = 0; i < request->requestslist_size(); i++) {
        reply->add_responselist();
    }

    std::vector<std::future<grpc::Status>> results;
    for (int i = 0; i < request->requestslist_size(); i++) {
        string reqId = requestId;
        reqId.append(std::to_string(i));

        string filePath = tmp_dir;
        filePath.append("/");
        filePath.append(request->requestslist(i).path());
        results.push_back(
            std::async(std::launch::async, &DafnyVerifierServiceImpl::VerifySingleRequest, this,
                reqId, filePath, false, &(request->requestslist(i)), reply->mutable_responselist(i))
            );
    }

    for(int i = 0; i < request->requestslist_size(); i++) {
        results[i].wait();
        if (!results[i].get().ok()) {
            return results[i].get();
        }
    }

    auto end = time_point_cast<milliseconds>(system_clock::now());
    auto end_from_epoch = end.time_since_epoch().count();
    reply->set_executiontimeinms(end_from_epoch - start_from_epoch);
    LOG("request %s processed; finished after %ld ms\n",
        requestId.c_str(), end_from_epoch - start_from_epoch);
    if (dir_to_delete != "") {
        std::string rm_cmd = "rm -rf ";
        rm_cmd.append(dir_to_delete);
        system(rm_cmd.c_str());
    }
    return Status::OK;
}

Status DafnyVerifierServiceImpl::VerifySingleRequest(
    string requestId,
    string codePath,
    bool globalLockAlreadyAcquired,
    const VerificationRequest *request,
    VerificationResponse *reply)
{
    if (request->code() != "") {
        Status status = WriteToFile(codePath, request->code());
        if (!status.ok()) {
            return status;
        }
    }
    int coreId = AcquireCountSem(globalLockAlreadyAcquired);
    string timeLimitStr;
    if (request->timeout() != "") {
        timeLimitStr = request->timeout();
    }
    else {
        timeLimitStr = "0";
    }
    int pipefd[2]; // Used for pipe between this process and its child
    if (pipe(pipefd) == -1)
    {
        ReleaseCountSem(coreId);
        perror("Creating pipe failed");
        return Status::CANCELLED;
    }
    fcntl(pipefd[1], F_SETPIPE_SZ, 1048576);

    LOG("request %s codePath=%s issued with timeLimit = %s, waiting for response\n", requestId.c_str(), codePath.c_str(), timeLimitStr.c_str());
    int pid = fork();
    if (pid == 0)
    {
        // child process
        close(pipefd[0]);
        int ret = dup2(pipefd[1], STDOUT_FILENO);
        if (ret == -1)
        {
            perror("dup2 failed on pipefd[1]");
            exit(0);
        }
        cpu_set_t mask;
        int status;

        CPU_ZERO(&mask);
        CPU_SET(coreId, &mask);
        status = sched_setaffinity(0, sizeof(mask), &mask);
        if (status != 0)
        {
            perror("sched_setaffinity");
            exit(0);
        }

        char **argv = new char *[request->arguments().size() + 5];
        argv[0] = strdup("timeout");
        argv[1] = strdup(timeLimitStr.c_str());
        argv[2] = strdup(dafnyBinaryPath.c_str());
        argv[3] = strdup(codePath.c_str());
        for (int i = 0; i < request->arguments().size(); i++)
        {
            argv[i + 4] = strdup(request->arguments(i).c_str());
        }
        argv[request->arguments().size() + 4] = NULL;
        execvp("timeout", argv);
        fflush(stdout);
        exit(0);
    }
    close(pipefd[1]);
    int stat;
    waitpid(pid, &stat, 0);
    LOG("request %s dafny finished executing exit_status=%d\n", requestId.c_str(), WEXITSTATUS(stat));
    // if (stat != 0) {
    //     std::cerr << "dafny failed for filepath=" << codePath << " with error: " << std::strerror(errno) << "\n";
    //     sem_post(&countSem);
    //     return Status::CANCELLED;
    // }

    char buffer[BUFFER_SIZE];
    string dafnyOutput = "";
    int len;
    do
    {
        len = read(pipefd[0], buffer, BUFFER_SIZE);
        LOG("request %s len=%d\n", requestId.c_str(), len);
        dafnyOutput.append(buffer, len);
    } while (len == BUFFER_SIZE);
    LOG("request %s response: %s\n", requestId.c_str(), dafnyOutput.c_str());
    reply->set_response(dafnyOutput);
    reply->set_filename(codePath);
    reply->set_exitstatus(WEXITSTATUS(stat));
    close(pipefd[0]);
    ReleaseCountSem(coreId);
    // sem_post(&countSem);
    return Status::OK;
}

Status DafnyVerifierServiceImpl::TwoStageVerify(ServerContext *context,
                                                const TwoStageRequest *request,
                                                VerificationResponseList *reply)
{
    struct stat info;
    std::string codePath;
    char *tmp_dir = NULL;
    std::string dir_to_delete = "";
    string requestId;

    if (request->directorypath() != "") {
        if (stat(request->directorypath().c_str(), &info) != 0) {
            std::string msg("Duplicating folder does not exist! directorypath = ");
            msg.append(request->directorypath());
            return Status(StatusCode::INVALID_ARGUMENT, msg);
        }
        else if (!(info.st_mode & S_IFDIR)) {
            std::string msg("Duplicating folder is not a folder! directorypath = ");
            msg.append(request->directorypath());
            return Status(StatusCode::INVALID_ARGUMENT, msg);
            return Status::CANCELLED;
        }
        char tmplate[] = "/dev/shm/verifier_dir_XXXXXX";
        tmp_dir = strdup(mkdtemp(tmplate));
        std::string copy_cmd = "cp -r ";
        copy_cmd.append(request->directorypath());
        copy_cmd.append("/* ");
        copy_cmd.append(tmp_dir);
        dir_to_delete.append(tmp_dir);
        system(copy_cmd.c_str());
        codePath = tmp_dir;
        codePath += "/";
        requestId = request->directorypath();
        requestId.append(&(tmp_dir[9]));
        requestId.push_back('_');
    }
    else {
        // sem_post(&countSem);
        std::string msg("Duplicating folder not given! directorypath == \"\"");
        return Status(StatusCode::INVALID_ARGUMENT, msg);
    }

    for (int i = 0; i < request->firststagerequestslist_size(); i++) {
        string path = codePath;
        path += request->firststagerequestslist(i).path();
        FILE *file = fopen(path.c_str(), "w");
        if (file == NULL)
        {
            // sem_post(&countSem);
            std::string msg = "Error opening file ";
            msg.append(path);
            perror(msg.c_str());
        
            return Status::CANCELLED;
        }
        // std::cerr << "Writing to " << path << "\n";
        fputs(request->firststagerequestslist(i).code().c_str(), file);
        fclose(file);
    }

    for (int i = 0; i < request->prerequisiteforsecondstagerequestslist_size(); i++) {
        reply->add_responselist();
    }

    if (request->runexclusive()) {
        LockGlobalMutex();
    }
    auto start = time_point_cast<milliseconds>(system_clock::now());
    auto start_from_epoch = start.time_since_epoch().count();

    std::vector<std::future<grpc::Status>> prerequisite_tasks_futures;
    std::vector<grpc::Status> prerequisite_tasks_results;
    for (int i = 0; i < request->prerequisiteforsecondstagerequestslist_size(); i++) {
        string reqId = requestId;
        reqId.append(std::to_string(i));

        string filePath = codePath;
        filePath.append(request->prerequisiteforsecondstagerequestslist(i).path());
        prerequisite_tasks_futures.push_back(
            std::async(std::launch::async, &DafnyVerifierServiceImpl::VerifySingleRequest, this,
                reqId, filePath, request->runexclusive(), 
                &(request->prerequisiteforsecondstagerequestslist(i)),
                reply->mutable_responselist(i))
            );
    }

    for(int i = 0; i < request->prerequisiteforsecondstagerequestslist_size(); i++) {
        prerequisite_tasks_futures[i].wait();
        prerequisite_tasks_results.push_back(prerequisite_tasks_futures[i].get());
    }

    bool prerequisite_status = true;
    for(int i = 0; i < request->prerequisiteforsecondstagerequestslist_size(); i++) {
        if (request->prerequisiteforsecondstagerequestslist(i).shouldpassnotfail()) {
            if (reply->mutable_responselist(i)->exitstatus() != 0) {
                LOG("shouldpass case: %dth response not okay\n", i);
                prerequisite_status = false;
                break;
            }
        } else {
            if (reply->mutable_responselist(i)->exitstatus() == 0) {
                LOG("shouldfail case: %dth response not okay\n", i);
                prerequisite_status = false;
                break;
            }
        }
    }
    LOG("after prerequisite tasks checks %d\n", prerequisite_status);
    Status ret_status = Status::OK;
    if (prerequisite_status) {
        for (int i = 0; i < request->secondstagerequestslist_size(); i++) {
            reply->add_responselist();
        }
        std::vector<std::future<grpc::Status>> futures;
        std::vector<grpc::Status> results;
        for (int i = 0; i < request->secondstagerequestslist_size(); i++) {
            string reqId = requestId;
            reqId.append(std::to_string(i + request->prerequisiteforsecondstagerequestslist_size()));

            string filePath = codePath;
            filePath.append(request->secondstagerequestslist(i).path());
            futures.push_back(
                std::async(std::launch::async, &DafnyVerifierServiceImpl::VerifySingleRequest, this,
                    reqId, filePath, request->runexclusive(), 
                    &(request->secondstagerequestslist(i)), 
                    reply->mutable_responselist(i + request->prerequisiteforsecondstagerequestslist_size()))
                );
        }

        for(int i = 0; i < request->secondstagerequestslist_size(); i++) {
            futures[i].wait();
            results.push_back(futures[i].get());
        }
        // std::cerr << "got all responses\n";
        for(int i = 0; i < request->secondstagerequestslist_size(); i++) {
            if (!results[i].ok()) {
                // std::cerr << i << "th response not okay" << results[i].get().error_message() << "\n";
                ret_status = results[i];
                break;
            }
        }
    }
    auto end = time_point_cast<milliseconds>(system_clock::now());
    auto end_from_epoch = end.time_since_epoch().count();
    if (request->runexclusive()) {
        UnlockGlobalMutex();
    }
    reply->set_executiontimeinms(end_from_epoch - start_from_epoch);
    LOG("request %s processed; finished after %ld ms\n",
        requestId.c_str(), end_from_epoch - start_from_epoch);
    if (dir_to_delete != "") {
        std::string rm_cmd = "rm -rf ";
        rm_cmd.append(dir_to_delete);
        system(rm_cmd.c_str());
    }
    return ret_status;
}

Status DafnyVerifierServiceImpl::Verify(ServerContext *context,
                                        const VerificationRequest *request,
                                        VerificationResponse *reply)
{
    // sem_wait(&countSem);
    std::string codePath;
    string requestId;
    auto start = time_point_cast<milliseconds>(system_clock::now());
    auto start_from_epoch = start.time_since_epoch().count();
    if (request->code() != "" && request->path() == "") {
        if (verbose) {
            std::istringstream f(request->code());
            std::getline(f, requestId);
            LOG("request %s received\n", requestId.c_str());
        }
        codePath = WriteToTmpFile(request);
    }
    else {
        if (request->code() == "" && request->path() == "") {
            std::string msg("Neither code nor path is given");
            return Status(StatusCode::INVALID_ARGUMENT, msg);
        }
        else if (request->code() == "") {
            if (verbose) {
                requestId = request->path();
                LOG("request %s received\n", requestId.c_str());
            }
            codePath = request->path();
        }
        else {
            if (verbose) {
                requestId = request->path();
                LOG("request %s received\n", requestId.c_str());
            }
            Status status = WriteToFile(request);
            if (!status.ok()) {
                return status;
            }
            codePath = request->path();
        }
    }

    Status ret_status = VerifySingleRequest(requestId, codePath, false, request, reply);
    auto end = time_point_cast<milliseconds>(system_clock::now());
    auto end_from_epoch = end.time_since_epoch().count();
    reply->set_executiontimeinms(end_from_epoch - start_from_epoch);
    LOG("request %s processed; finished after %ld ms\n",
        requestId.c_str(), end_from_epoch - start_from_epoch);
    return ret_status;
}