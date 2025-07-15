cmake_minimum_required(VERSION 3.10)
project(chat)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g")

# 设置 MySQL Connector 头文件路径
set(MYSQL_CONNECTOR_INCLUDE_DIR "/usr/include/mysql-cppconn")

# 找依赖库
find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)
find_package(Boost REQUIRED COMPONENTS filesystem)
find_package(Boost REQUIRED COMPONENTS filesystem log log_setup thread date_time)


# 查找 hiredis 库
find_library(HIREDIS_LIBRARY NAMES hiredis REQUIRED)

# 查找 MySQL Connector 库（优先使用可能的多个名字）
find_library(MYSQL_CONNECTOR_LIB
  NAMES mysqlcppconn10 mysqlcppconn8 mysqlcppconn
  PATHS /usr/lib64
)

# 源文件
file(GLOB SRC
  "${CMAKE_SOURCE_DIR}/src/*.cpp"
)

# 可执行文件
add_executable(server ${SRC})

# 头文件包含目录
target_include_directories(server PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_BINARY_DIR}                    
  ${Protobuf_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS}
  ${gRPC_INCLUDE_DIRS}
  ${MYSQL_CONNECTOR_INCLUDE_DIR}
)

# 链接库
target_link_libraries(server
  PRIVATE
    ${Protobuf_LIBRARIES}
    gRPC::grpc++
    gRPC::grpc++_reflection
    Boost::filesystem
    ${HIREDIS_LIBRARY}
    ${MYSQL_CONNECTOR_LIB}
    Boost::log Boost::log_setup Boost::thread Boost::date_time
)
