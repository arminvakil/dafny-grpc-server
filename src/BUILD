load("@rules_cc//cc:defs.bzl", "cc_binary")

cc_library(
    name = "verifier_service",
    srcs = ["verifier_service.cpp"],
    hdrs = ["verifier_service.h"],
    deps = [
        "//protos:verifier_cc_grpc",
        "@com_github_grpc_grpc//:grpc++",
        "@com_google_absl//absl/strings",
    ],
    linkopts = ["-pthread -g -lpthread"],
)

cc_binary(
    name = "server",
    srcs = ["main.cpp"],
    deps = [
        ":verifier_service",
        "//protos:verifier_cc_grpc",
        "@com_github_grpc_grpc//:grpc++",
    ],
    linkopts = ["-g"],
)