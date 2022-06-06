load("@rules_cc//cc:defs.bzl", "cc_proto_library")
load("@rules_proto//proto:defs.bzl", "proto_library")
load("@com_github_grpc_grpc//bazel:cc_grpc_library.bzl", "cc_grpc_library")

package(default_visibility = ["//visibility:public"])

proto_library(
    name = "verifier_proto",
    srcs = ["verifier.proto"],
)

cc_proto_library(
    name = "verifier_cc_proto",
    deps = [":verifier_proto"],
)

cc_grpc_library(
    name = "verifier_cc_grpc",
    srcs = [":verifier_proto"],
    grpc_only = True,
    deps = [":verifier_cc_proto"],
)