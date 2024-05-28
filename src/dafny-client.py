"""The Python implementation of the gRPC dafny server client."""

from __future__ import print_function

import logging
import random
import sys
import numpy as np

from google.protobuf.internal import type_checkers
from google.protobuf import descriptor
from google.protobuf.json_format import Parse, ParseDict
# from google.protobuf import text_format
# from google.protobuf import text_encoding

import grpc
import verifier_pb2
import verifier_pb2_grpc
import dafnyArgs_pb2
import dafnyArgs_pb2_grpc

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

def twoStageVerify(stub, base_dir):
    request = verifier_pb2.TwoStageRequest()
    request.directoryPath = base_dir.path
    for i in np.arange(1):
        verification_request = verifier_pb2.VerificationRequest()
        verification_request.arguments.append('/compile:0')
        verification_request.arguments.append('/rlimit:50000')
        # verification_request.arguments.append('/allowGlobals')
        verification_request.arguments.append('/arith:5')
        verification_request.arguments.append('/trace')
        verification_request.arguments.append('/noCheating:1')
        verification_request.timeout = "20m"
        verification_request.path = "Distributed/Protocol/SHT/RefinementProof/InvProof.i.dfy"
        request.secondStageRequestsList.append(verification_request)
    response = stub.TwoStageVerify(request)
    print(f"Received response is {response}")

def verify(tasksList):
    with grpc.insecure_channel(sys.argv[1]) as channel:
        stub = verifier_pb2_grpc.DafnyVerifierServiceStub(channel)
        tmp_dir = verifier_pb2.TmpFolder()
        tmp_dir.path = "/proj/vm-vm-PG0/BASE-DIRECTORY/IroncladImproved/ironfleet/src/Dafny/"
        request = verifier_pb2.TwoStageRequest()
        request.directoryPath = tmp_dir.path
        for f in tasksList.tasks:
            verification_request = verifier_pb2.VerificationRequest()
            for arg in f.arguments:
                verification_request.arguments.append(arg)
            verification_request.timeout = "10s"
            verification_request.path = f.path
            request.secondStageRequestsList.append(verification_request)
        # print(request)
        response = stub.TwoStageVerify(request)
        print(f"Received response is {response}")

def run():
    with grpc.insecure_channel(sys.argv[1]) as channel:
        stub = verifier_pb2_grpc.DafnyVerifierServiceStub(channel)
        tmp_dir = verifier_pb2.TmpFolder()
        tmp_dir.path = "/dev/shm/verifier_dir_tmp/"
        # new_dir = duplicateDir(stub, tmp_dir)
        twoStageVerify(stub, tmp_dir)


if __name__ == '__main__':
    if len(sys.argv) < 3:
        print (f'Usage: {sys.argv[0]} IP:PORT PATH')
        sys.exit(1)
    logging.basicConfig()
    tasksList = dafnyArgs_pb2.TasksList()
    # Read the existing address book.
    with open(sys.argv[2], "r") as f:
        tasksList = Parse(f.read(), tasksList)
        # tasksList.Parse(f.read())
    verify(tasksList)
    # run()
