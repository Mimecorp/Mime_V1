cc_library(
    name = "ekf",
    srcs = ["ekf.cpp"],
    hdrs = ["ekf.h"],
    deps = ["@eigen//:eigen"],
)

cc_library(
    name = "logger",
    srcs = ["logger.cpp"],
    hdrs = ["logger.h"],
)

cc_binary(
    name = "server",
    srcs = ["server.cpp"],
    deps = [
        ":ekf",
        "//proto:telemetry_cc_grpc",
        "//proto:telemetry_cc_proto",
        "@com_github_grpc_grpc//:grpc++",
    ],
)
