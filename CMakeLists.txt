cmake_minimum_required(VERSION 3.10)
project(chat)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -std=c++20")

list(APPEND CMAKE_PREFIX_PATH "/usr/local/lib/cmake/grpc")

# 查找依赖
find_package(Protobuf REQUIRED)
find_package(Boost REQUIRED COMPONENTS filesystem)
find_package(gRPC REQUIRED)

# 生成 Protobuf 代码
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS message.proto)

# 收集源文件
aux_source_directory(. SRC)
list(APPEND SRC ${PROTO_SRCS} ${GRPC_SRCS})

# 创建可执行文件
add_executable(server ${SRC})

# 链接库
target_link_libraries(server
  PRIVATE
  ${Protobuf_LIBRARIES}
  Boost::filesystem
  gRPC::grpc++
  gRPC::grpc++_reflection
)

# 头文件目录
target_include_directories(server
  PRIVATE
  ${Protobuf_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS}
  ${gRPC_INCLUDE_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR}
)