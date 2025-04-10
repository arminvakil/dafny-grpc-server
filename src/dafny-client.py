"""The Python implementation of the gRPC dafny server client."""

from __future__ import print_function

import logging
import random
import sys
import numpy as np
import threading
from concurrent.futures import ThreadPoolExecutor 

from google.protobuf.internal import type_checkers
from google.protobuf import descriptor
from google.protobuf.json_format import Parse, ParseDict
# from google.protobuf import text_format
# from google.protobuf import text_encoding

import grpc
import verifier_pb2
import verifier_pb2_grpc

correct = 0
incorrect = 0
def single_verify(stub, path):
    request = verifier_pb2.VerificationRequest()
    with open(path) as f:
        request.code = "".join(f.readlines())
    request.arguments.append('/compile:0')
    request.arguments.append('/rlimit:1000000')
    request.arguments.append('/trace')
    request.arguments.append('/noCheating:1')
    request.timeout = "10s"
    # print(request)
    # print(request)
    response = stub.Verify(request)
    if response.response[-9:-1] == b"0 errors":
        return True
    else:
        print(response.response)
        return False

def verify(path):
    with grpc.insecure_channel(sys.argv[1]) as channel:
        stub = verifier_pb2_grpc.DafnyVerifierServiceStub(channel)
        executor = ThreadPoolExecutor(max_workers = 10)
        result_list = []
        for i in np.arange(0, 10):
            result_list.append(executor.submit(single_verify, stub, path))
        for r in result_list:
            print(r.result())
            # print(f"Received response is {response}")
    print("correct = ", correct)
    print("incorrect = ", incorrect)

if __name__ == '__main__':
    if len(sys.argv) < 3:
        print (f'Usage: {sys.argv[0]} IP:PORT PATH')
        sys.exit(1)
    logging.basicConfig()
    verify(sys.argv[2])
