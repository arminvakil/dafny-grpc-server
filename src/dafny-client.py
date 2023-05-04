"""The Python implementation of the gRPC dafny server client."""

from __future__ import print_function

import logging
import random
import sys
import numpy as np

import grpc
import verifier_pb2
import verifier_pb2_grpc

def getTempDir(stub):
    empty_request = verifier_pb2.Empty()
    response = stub.CreateTmpFolder(empty_request, timeout=1200)
    print(f"Received tmp directory is {response.path}")
    return response

def duplicateDir(stub, tmp_dir):
    response = stub.DuplicateFolder(tmp_dir, timeout=1200)
    print(f"Received duplicate directory is {response.path}")
    return response

def verify(stub):
    verification_request = verifier_pb2.VerificationRequest()
    with open(sys.argv[2], 'r') as f:
        verification_request.code = f.read()
    verification_request.arguments.append('/compile:0')
    verification_request.arguments.append('/rlimit:100000')
    verification_request.arguments.append('/allowGlobals')
    verification_request.arguments.append('/noNLarith')
    verification_request.arguments.append('/autoTriggers:1')
    verification_request.arguments.append('/verifyAllModules')
    verification_request.arguments.append('/exitAfterFirstError')
    response = stub.Verify(verification_request, timeout=1200)
    print(f"Received response is {response.response}")
    return response


def cloneAndVerify(stub, base_dir):
    request = verifier_pb2.CloneAndVerifyRequest()
    request.directoryPath = base_dir.path
    for i in np.arange(100):
        verification_request = verifier_pb2.VerificationRequest()
        verification_request.arguments.append('/compile:0')
        verification_request.arguments.append('/rlimit:100000')
        verification_request.arguments.append('/allowGlobals')
        verification_request.arguments.append('/noNLarith')
        verification_request.arguments.append('/autoTriggers:1')
        verification_request.timeout = "1m"
        verification_request.path = "Distributed/Protocol/SHT/Host.i.dfy"
        request.requestsList.append(verification_request)
    response = stub.CloneAndVerify(request, timeout=1200)
    print(f"Received response is {response}")

def run():
    with grpc.insecure_channel(sys.argv[1]) as channel:
        stub = verifier_pb2_grpc.DafnyVerifierServiceStub(channel)
        tmp_dir = verifier_pb2.TmpFolder()
        tmp_dir.path = "/dev/shm/verifier_dir_tmp/"
        # new_dir = duplicateDir(stub, tmp_dir)
        cloneAndVerify(stub, tmp_dir)


if __name__ == '__main__':
    if len(sys.argv) < 2:
        print (f'Usage: {sys.argv[0]} IP:PORT PATH')
        sys.exit(1)
    logging.basicConfig()
    run()
