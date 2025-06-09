#pragma once
#include "const.h"

/**
 * @brief HTTP连接处理类，管理单个客户端HTTP连接的生命周期
 * 基于Boost.Beast实现，支持异步读写、请求处理和超时管理
 */
class HttpConnection : public std::enable_shared_from_this<HttpConnection>
{
    friend class LogicSystem; // 允许LogicSystem访问私有成员
public:
    /**
     * @brief 构造函数，接收已连接的socket
     * @param socket 从acceptor获取的TCP socket（右值引用，移动语义）
     */
    HttpConnection(tcp::socket socket);

    /**
     * @brief 启动连接处理，触发异步读取请求
     * 调用后开始接收HTTP请求，后续流程由异步回调驱动
     */
    void start();

private:
    /**
     * @brief 检查连接超时
     * 定时检查连接是否超时，超时则关闭socket
     */
    void checkDeadline();

    /**
     * @brief 将HTTP响应异步写入socket
     * 处理完成后自动管理连接生命周期（保持或关闭）
     */
    void writeResponse();

    /**
     * @brief 处理HTTP请求内容
     * 解析请求路径、方法和参数，生成对应的响应内容
     */
    void handleReq();

    // 私有成员变量
    tcp::socket _socket;                  // 与客户端通信的TCP套接字
    beast::flat_buffer _buffer{8192};     // 接收HTTP数据的缓冲区（初始大小8KB）
    http::request<http::dynamic_body> _request;  // 存储HTTP请求内容
    http::response<http::dynamic_body> _response; // 存储待发送的HTTP响应
    net::steady_timer _deadlineTimer{     // 连接超时计时器
        _socket.get_executor(),           // 使用socket的执行器
        std::chrono::seconds(60)          // 默认超时时间60秒
    };
};