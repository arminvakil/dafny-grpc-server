#include <assert.h>
#include <sstream>
#include <sys/wait.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <cstdlib>

#include "verifier_service.h"

DafnyVerifierServiceImpl::DafnyVerifierServiceImpl(int num_workers) : numWorkers(num_workers)
{
    sem_init(&countSem, 0, numWorkers);
    // logFile = fopen("/tmp/log.txt", "w");
    pthread_mutex_init(&logFileLock, NULL);
}

Status DafnyVerifierServiceImpl::Verify(ServerContext *context,
                                        const VerificationRequest *request,
                                        VerificationResponse *reply)
{
    auto start = std::chrono::system_clock::now();
    std::time_t start_time = std::chrono::system_clock::to_time_t(start);
    reply->set_starttime(start_time);
    sem_wait(&countSem);
    std::string tmpFileName = std::tmpnam(nullptr);
    FILE *file = fopen(tmpFileName.c_str(), "w");
    if (file == NULL)
    {
        perror("Error opening temporary file");
        return Status::CANCELLED;
    }
    reply->set_filename(tmpFileName);
    fputs(request->code().c_str(), file);
    fclose(file);
    int pid = fork();
    if (pid == 0)
    {
        // child process
        sleep(10);
        exit(0);
    }
    int stat;
    waitpid(pid, &stat, 0);
    reply->set_response("test");
    sem_post(&countSem);
    auto end = std::chrono::system_clock::now();
    std::time_t end_time = std::chrono::system_clock::to_time_t(end);
    std::chrono::duration<float> elapsed_seconds = end - start;
    reply->set_executiontime(elapsed_seconds.count());
    // std::istringstream f(request->code());
    // std::string firstLine;
    // std::getline(f, firstLine);
    // pthread_mutex_lock(&logFileLock);
    // logFile = fopen("/tmp/log.txt", "a");
    // fprintf(logFile, "request %s processed; finished at %s elapsed seconds=%fs\n", 
    //   firstLine.c_str(),
    //   std::ctime(&end_time), elapsed_seconds.count());
    // fclose(logFile);
    // pthread_mutex_unlock(&logFileLock);
    return Status::OK;
}