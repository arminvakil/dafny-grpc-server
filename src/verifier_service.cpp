#include <assert.h>
#include <sstream>
#include <sys/wait.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <cstdlib>

#include "verifier_service.h"
#define BUFFER_SIZE 10000

using namespace std;

#define LOG(...) \
  if (verbose) { \
    pthread_mutex_lock(&logFileLock); \
    logFile = fopen("/tmp/log.txt", "a"); \
    fprintf(logFile, __VA_ARGS__); \
    fclose(logFile); \
    pthread_mutex_unlock(&logFileLock); \
  }

DafnyVerifierServiceImpl::DafnyVerifierServiceImpl(int num_workers, string dafny_binary_path) : numWorkers(num_workers), dafnyBinaryPath(dafny_binary_path)
{
    sem_init(&countSem, 1, numWorkers);
    pthread_mutex_init(&logFileLock, 0);
    logFile = fopen("/tmp/log.txt", "w");
    fclose(logFile);
}

Status DafnyVerifierServiceImpl::Verify(ServerContext *context,
                                        const VerificationRequest *request,
                                        VerificationResponse *reply)
{
    sem_wait(&countSem);
    std::istringstream f(request->code());
    std::string firstLine;
    std::getline(f, firstLine);
    LOG("request %s received\n", firstLine.c_str());
    auto start = std::chrono::system_clock::now();
    std::time_t start_time = std::chrono::system_clock::to_time_t(start);
    reply->set_starttime(start_time);
    std::string tmpFileName = std::tmpnam(nullptr);
    tmpFileName.append(".dfy");
    FILE *file = fopen(tmpFileName.c_str(), "w");
    if (file == NULL)
    {
        perror("Error opening temporary file");
        return Status::CANCELLED;
    }
    reply->set_filename(tmpFileName);
    fputs(request->code().c_str(), file);
    fclose(file);

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
        char **argv = new char *[request->arguments().size() + 3];
        argv[0] = strdup(dafnyBinaryPath.c_str());
        argv[1] = strdup(tmpFileName.c_str());
        for (int i = 0; i < request->arguments().size(); i++)
        {
            argv[i + 2] = strdup(request->arguments(i).c_str());
        }
        argv[request->arguments().size() + 2] = NULL;
        execvp(dafnyBinaryPath.c_str(), argv);
        exit(0);
    }
    int stat;
    LOG("request %s issued, waiting for response\n", firstLine.c_str());
    waitpid(pid, &stat, 0);
    // if (stat != 0) {
    //     std::cerr << "dafny failed " << std::strerror(errno) << "\n";
    //     return Status::CANCELLED;
    // }
    LOG("request %s dafny finished executing\n", firstLine.c_str());

    char buffer[BUFFER_SIZE];
    string dafnyOutput = "";
    int len;
    do
    {
        len = read(pipefd[0], buffer, BUFFER_SIZE);
        LOG("request %s len=%d\n", firstLine.c_str(), len);
        dafnyOutput.append(buffer, len);
    } while (len == BUFFER_SIZE);
    LOG("request %s response: %s\n", firstLine.c_str(), dafnyOutput.c_str());
    reply->set_response(dafnyOutput);
    close(pipefd[0]);
    close(pipefd[1]);
    sem_post(&countSem);
    auto end = std::chrono::system_clock::now();
    std::chrono::duration<float> elapsed_seconds = end - start;
    reply->set_executiontime(elapsed_seconds.count());
    std::time_t end_time = std::chrono::system_clock::to_time_t(end);
    LOG("request %s processed; finished at %s elapsed seconds=%fs\n",
            firstLine.c_str(), std::ctime(&end_time), elapsed_seconds.count());
    return Status::OK;
}