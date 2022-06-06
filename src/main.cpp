#include <assert.h>
#include <iostream>
#include <pthread.h>

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
DafnyVerifierServiceImpl *verifier_service;

void usage(char **argv)
{
    std::cout << "Usage: " << argv[0] << " -i IP:PORT\n";
}

int main(int argc, char **argv)
{
    char opt;

    while ((opt = getopt(argc, argv, "hvi")) != -1)
    {
        switch (opt)
        {
        case 'h':
            usage(argv);
            exit(0);
        case 'v':
            verbose = true;
            break;
        case 'i':
            ipPort = atoi(optarg);
            break;
        default:
            std::cerr << "Illegal command line option '" << opt << "'" << endl;
            usage(argv);
            exit(-1);
        }
    }

    verifier_service = new DafnyVerifierServiceImpl(40);

    ServerBuilder builder;
    builder.AddListeningPort(ipPort, grpc::InsecureServerCredentials());
    builder.RegisterService(dynamic_cast<grpc::Service *>(verifier_service));
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cerr << "Server listening on " << ipPort << std::endl;
    server->Wait();
}