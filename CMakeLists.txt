cmake_minimum_required(VERSION 3.10)
project(chat)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -std=c++20")
set(MYSQL_CONNECTOR_INCLUDE_DIR "/usr/include/mysql-cppconn")

list(APPEND CMAKE_PREFIX_PATH "/usr/local/lib/cmake/grpc")

# 查找依赖
find_package(Protobuf REQUIRED)
find_package(Boost REQUIRED COMPONENTS filesystem)
find_package(gRPC REQUIRED)

# -------------- 修改：查找 MySQL Connector/C++ 库 --------------
find_library(MYSQL_CONNECTOR_LIB
  NAMES mysqlcppconn10 mysqlcppconn8 mysqlcppconn  # 按优先级列出可能的库名
  PATHS /usr/lib64  # 指定库文件所在目录（根据你的 find 结果）
  REQUIRED
)
# ----------------------------------------------------------------

# 生成 Protobuf 代码
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS message.proto)

# 收集源文件
aux_source_directory(. SRC)
list(APPEND SRC ${PROTO_SRCS} ${GRPC_SRCS})

# 创建可执行文件
add_executable(server ${SRC})

# 查找 hiredis 库
find_library(HIREDIS_LIBRARY NAMES hiredis REQUIRED)

# -------------- 修改：链接 mysqlcppconn 而非 mysqlclient --------------
target_link_libraries(server
  PRIVATE
  ${Protobuf_LIBRARIES}
  Boost::filesystem
  gRPC::grpc++
  gRPC::grpc++_reflection
  ${HIREDIS_LIBRARY}
  ${MYSQL_CONNECTOR_LIB}  # 链接 MySQL Connector/C++ 库
  # ${MYSQL_CLIENT_LIBRARY}  # 注释掉或删除这行，避免链接错误的库
)
# ----------------------------------------------------------------

# 头文件目录
target_include_directories(server
  PRIVATE
  ${Protobuf_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS}
  ${gRPC_INCLUDE_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${MYSQL_CONNECTOR_INCLUDE_DIR}  # 已包含 jdbc/ 目录
)