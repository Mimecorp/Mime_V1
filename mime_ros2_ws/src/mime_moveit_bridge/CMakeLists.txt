cmake_minimum_required(VERSION 3.8)
project(mime_moveit_bridge)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(geometry_msgs REQUIRED)

# Need to find gRPC and Protobuf
find_package(Protobuf REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(gRPC REQUIRED grpc++)
find_package(Threads REQUIRED)

set(PROTO_FILE "${CMAKE_CURRENT_SOURCE_DIR}/proto/telemetry.proto")
set(PROTO_SRC "${CMAKE_CURRENT_BINARY_DIR}/telemetry.pb.cc")
set(PROTO_HDR "${CMAKE_CURRENT_BINARY_DIR}/telemetry.pb.h")
set(GRPC_SRC "${CMAKE_CURRENT_BINARY_DIR}/telemetry.grpc.pb.cc")
set(GRPC_HDR "${CMAKE_CURRENT_BINARY_DIR}/telemetry.grpc.pb.h")

# Get grpc_cpp_plugin
find_program(gRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)

add_custom_command(
    OUTPUT ${PROTO_SRC} ${PROTO_HDR} ${GRPC_SRC} ${GRPC_HDR}
    COMMAND protobuf::protoc
    ARGS --grpc_out=${CMAKE_CURRENT_BINARY_DIR}
         --cpp_out=${CMAKE_CURRENT_BINARY_DIR}
         -I${CMAKE_CURRENT_SOURCE_DIR}/proto
         --plugin=protoc-gen-grpc=${gRPC_CPP_PLUGIN_EXECUTABLE}
         ${PROTO_FILE}
    DEPENDS ${PROTO_FILE}
)

add_library(telemetry_grpc_proto STATIC ${PROTO_SRC} ${GRPC_SRC})
target_include_directories(telemetry_grpc_proto PUBLIC ${CMAKE_CURRENT_BINARY_DIR} ${gRPC_INCLUDE_DIRS})
target_link_libraries(telemetry_grpc_proto PUBLIC ${gRPC_LIBRARIES} protobuf::libprotobuf)

add_executable(moveit_bridge_node src/moveit_bridge_node.cpp)
target_include_directories(moveit_bridge_node PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
  ${CMAKE_CURRENT_BINARY_DIR})

ament_target_dependencies(moveit_bridge_node
  "rclcpp"
  "geometry_msgs"
)

target_link_libraries(moveit_bridge_node telemetry_grpc_proto)

install(TARGETS
  moveit_bridge_node
  DESTINATION lib/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
