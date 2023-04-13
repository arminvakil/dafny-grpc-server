#include <assert.h>
#include <sstream>
#include <sys/wait.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <cstdlib>
#include <sys/types.h>
#include <sys/stat.h>

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
    pthread_mutex_init(&logFileLock, 0);
    logFile = fopen("/tmp/log.txt", "w");
    fclose(logFile);
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
    reply->set_path(tmp_dir);
    return Status::OK;
}

std::string DafnyVerifierServiceImpl::WriteToTmpFile(const CloneAndVerifyRequest *request)
{
    std::string tmpFileName = std::tmpnam(nullptr);
    tmpFileName.append(".dfy");
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

Status WriteToFile(const CloneAndVerifyRequest *request)
{
    FILE *file = fopen(request->modifyingfile().c_str(), "w");
    if (file == NULL)
    {
        std::string msg = "Error opening file ";
        msg.append(request->modifyingfile());
        perror(msg.c_str());
        return Status::CANCELLED;
    }
    fputs(request->code().c_str(), file);
    fclose(file);
    return Status::OK;
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
                                                VerificationResponse *reply)
{
    int val;
    sem_getvalue(&countSem, &val);
    // std::cerr << "sem_value: " << val << "\n";
    sem_wait(&countSem);
    struct stat info;
    std::string codePath;
    char *tmp_dir = NULL;
    std::string dir_to_delete = "";
    string requestId;
    auto start = time_point_cast<milliseconds>(system_clock::now());
    auto start_from_epoch = start.time_since_epoch().count();

    if (request->directorypath() != "") {
        if (stat(request->directorypath().c_str(), &info) != 0) {
            sem_post(&countSem);
            std::string msg("Duplicating folder does not exist! directorypath = ");
            msg.append(request->directorypath());
            return Status(StatusCode::INVALID_ARGUMENT, msg);
        }
        else if (!(info.st_mode & S_IFDIR)) {
            sem_post(&countSem);
            std::string msg("Duplicating folder is not a folder! directorypath = ");
            msg.append(request->directorypath());
            return Status(StatusCode::INVALID_ARGUMENT, msg);
            return Status::CANCELLED;
        }
        char tmplate[] = "/dev/shm/verifier_dir_XXXXXX";
        tmp_dir = mkdtemp(tmplate);
        std::string copy_cmd = "cp -r ";
        copy_cmd.append(request->directorypath());
        copy_cmd.append("/* ");
        copy_cmd.append(tmp_dir);
        dir_to_delete.append(tmp_dir);
        system(copy_cmd.c_str());
        codePath = tmp_dir;
        codePath += "/" + request->modifyingfile();
    }
    else {
        if (request->code() != "" && request->modifyingfile() == "") {
            if (verbose) {
                std::istringstream f(request->code());
                std::getline(f, requestId);
                LOG("request %s received\n", requestId.c_str());
            }
            codePath = WriteToTmpFile(request);
            dir_to_delete = codePath;
        }
        else {
            if (request->code() == "" && request->modifyingfile() == "") {
                std::string msg("Neither code nor path is given");
                return Status(StatusCode::INVALID_ARGUMENT, msg);
            }
            else if (request->code() == "") {
                if (verbose) {
                    requestId = request->modifyingfile();
                    LOG("request %s received\n", requestId.c_str());
                }
                codePath = request->modifyingfile();
            }
            else {
                if (verbose) {
                    requestId = request->modifyingfile();
                    LOG("request %s received\n", requestId.c_str());
                }
                Status status = WriteToFile(request);
                if (!status.ok()) {
                    return status;
                }
                codePath = request->modifyingfile();
            }
        }
    }

    FILE *file = fopen(codePath.c_str(), "w");
    if (file == NULL)
    {
        sem_post(&countSem);
        std::string msg = "Error opening file ";
        msg.append(codePath);
        perror(msg.c_str());
        
        return Status::CANCELLED;
    }
    fputs(request->code().c_str(), file);
    fclose(file);

    int pipefd[2]; // Used for pipe between this process and its child
    if (pipe(pipefd) == -1)
    {
        sem_post(&countSem);
        perror("Creating pipe failed");
        return Status::CANCELLED;
    }

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
        char **argv = new char *[request->arguments().size() + 5];
        argv[0] = strdup("timeout");
        if (request->timeout() != "") {
            argv[1] = strdup(request->timeout().c_str());
        } else {
            argv[1] = strdup("0");
        }
        argv[2] = strdup(dafnyBinaryPath.c_str());
        argv[3] = strdup(codePath.c_str());
        for (int i = 0; i < request->arguments().size(); i++)
        {
            argv[i + 4] = strdup(request->arguments(i).c_str());
        }
        argv[request->arguments().size() + 4] = NULL;
        execvp("timeout", argv);
        exit(0);
    }
    int stat;
    LOG("request %s issued, waiting for response\n", requestId.c_str());
    waitpid(pid, &stat, 0);
    // if (stat != 0) {
    //     std::cerr << "dafny failed " << std::strerror(errno) << "\n";
    //     return Status::CANCELLED;
    // }
    LOG("request %s dafny finished executing\n", requestId.c_str());

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
    close(pipefd[0]);
    close(pipefd[1]);
    sem_getvalue(&countSem, &val);
    // std::cerr << "sem_value at exit: " << val << "\n";
    sem_post(&countSem);
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

Status DafnyVerifierServiceImpl::Verify(ServerContext *context,
                                        const VerificationRequest *request,
                                        VerificationResponse *reply)
{
    sem_wait(&countSem);
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

    int pipefd[2]; // Used for pipe between this process and its child
    if (pipe(pipefd) == -1)
    {
        perror("Creating pipe failed");
        return Status::CANCELLED;
    }

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
        char **argv = new char *[request->arguments().size() + 5];
        argv[0] = strdup("timeout");
        if (request->timeout() != "") {
            argv[1] = strdup(request->timeout().c_str());
        } else {
            argv[1] = strdup("0");
        }
        argv[2] = strdup(dafnyBinaryPath.c_str());
        argv[3] = strdup(codePath.c_str());
        for (int i = 0; i < request->arguments().size(); i++)
        {
            argv[i + 4] = strdup(request->arguments(i).c_str());
        }
        argv[request->arguments().size() + 4] = NULL;
        execvp("timeout", argv);
        exit(0);
    }
    int stat;
    LOG("request %s issued, waiting for response\n", requestId.c_str());
    waitpid(pid, &stat, 0);
    // if (stat != 0) {
    //     std::cerr << "dafny failed " << std::strerror(errno) << "\n";
    //     return Status::CANCELLED;
    // }
    LOG("request %s dafny finished executing\n", requestId.c_str());

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
    close(pipefd[0]);
    close(pipefd[1]);
    sem_post(&countSem);
    auto end = time_point_cast<milliseconds>(system_clock::now());
    auto end_from_epoch = end.time_since_epoch().count();
    reply->set_executiontimeinms(end_from_epoch - start_from_epoch);
    LOG("request %s processed; finished after %ld ms\n",
        requestId.c_str(), end_from_epoch - start_from_epoch);
    return Status::OK;
}