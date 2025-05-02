#include <assert.h>
#include <iostream>
#include <pthread.h>
#include <thread>

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <iostream>

#include <grpcpp/grpcpp.h>

#include "verifier_service.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using namespace std;

using namespace DafnyExecutorServer;

bool verbose = false;
string ipPort = "0.0.0.0:50051";
string dafnyBinaryPath = "";
string baseDir = "";
DafnyVerifierServiceImpl *verifier_service;

void usage(char **argv)
{
    std::cout << "Usage: " << argv[0] << " -d DAFNY_EXE_PATH -j NUM_PHYSICAL_CORES [-i IP:PORT]\n";
}

int main(int argc, char **argv)
{
    char opt;
    int processor_count = -1;

    while ((opt = getopt(argc, argv, "hvi:d:j:n:")) != -1)
    {
        switch (opt)
        {
        case 'h':
            usage(argv);
            exit(0);
        case 'v':
            verbose = true;
            break;
        case 'd':
            dafnyBinaryPath = string(optarg);
            break;
        case 'i':
            ipPort = string(optarg);
            break;
        case 'j':
            processor_count = atoi(optarg);
            break;
        case 'n':
            baseDir = string(optarg);
            break;
        default:
            std::cerr << "Illegal command line option '" << opt << "'" << endl;
            usage(argv);
            exit(-1);
        }
    }
    if (dafnyBinaryPath == "") {
        std::cerr << "Dafny binary path must be specified" << endl;
        usage(argv);
        exit(-1);
    }
    if (processor_count == -1) {
        std::cerr << "processor count must be specified" << endl;
        usage(argv);
        exit(-1);
    }
    if (baseDir == "") {
        baseDir = "/dev/shm";
    }
    verifier_service = new DafnyVerifierServiceImpl(processor_count, dafnyBinaryPath, baseDir);

    ServerBuilder builder;
    builder.AddListeningPort(ipPort, grpc::InsecureServerCredentials());
    builder.RegisterService(dynamic_cast<grpc::Service *>(verifier_service));
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cerr << "Server listening on " << ipPort << std::endl;
    server->Wait();
}