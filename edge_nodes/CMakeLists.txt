cmake_minimum_required(VERSION 3.16)
project(MimeEdgeNodes VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Protobuf/gRPC using pkg-config (Ubuntu standard)
find_package(PkgConfig REQUIRED)
pkg_check_modules(PROTOBUF REQUIRED protobuf)
pkg_check_modules(GRPC REQUIRED grpc++)

# Find the compiler executables manually since pkg-config doesn't expose them
find_program(_PROTOBUF_PROTOC protoc)
find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)

if(NOT _PROTOBUF_PROTOC OR NOT _GRPC_CPP_PLUGIN_EXECUTABLE)
    message(FATAL_ERROR "protoc or grpc_cpp_plugin not found! Please check your installations.")
endif()

# Setup directories for generated code
set(PROTO_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../central_engine/proto")
set(PROTO_FILE "${PROTO_SRC_DIR}/telemetry.proto")
set(PROTO_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY ${PROTO_OUT_DIR})

# Ask protoc to generate C++ and gRPC files
add_custom_command(
    OUTPUT "${PROTO_OUT_DIR}/telemetry.pb.cc"
           "${PROTO_OUT_DIR}/telemetry.pb.h"
           "${PROTO_OUT_DIR}/telemetry.grpc.pb.cc"
           "${PROTO_OUT_DIR}/telemetry.grpc.pb.h"
    COMMAND ${_PROTOBUF_PROTOC}
    ARGS --grpc_out="${PROTO_OUT_DIR}"
         --cpp_out="${PROTO_OUT_DIR}"
         --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
         -I "${PROTO_SRC_DIR}"
         "${PROTO_FILE}"
    DEPENDS "${PROTO_FILE}"
)

add_executable(mime_edge_node 
    src/main.cpp 
    "${PROTO_OUT_DIR}/telemetry.pb.cc"
    "${PROTO_OUT_DIR}/telemetry.grpc.pb.cc"
)

# Tell the compiler where to find our include folders AND the generated proto files
target_include_directories(mime_edge_node PRIVATE 
    include
    "${PROTO_OUT_DIR}"
    ${GRPC_INCLUDE_DIRS}
    ${PROTOBUF_INCLUDE_DIRS}
)

# Link the gRPC and Protobuf system libraries found via pkg-config
target_link_options(mime_edge_node PRIVATE ${GRPC_LDFLAGS} ${PROTOBUF_LDFLAGS})
target_link_libraries(mime_edge_node 
    ${GRPC_LIBRARIES}
    ${PROTOBUF_LIBRARIES}
)
