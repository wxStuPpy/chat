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
    HttpConnection(boost::asio::io_context &ioc);
    void start();
    tcp::socket &getSocket();

private:
    void checkDeadline();
    void writeResponse();
    void handleReq();
    void preParseGetParam();

    // 私有成员变量
    tcp::socket _socket;                  // 与客户端通信的TCP套接字
    beast::flat_buffer _buffer{8192};     // 接收HTTP数据的缓冲区（初始大小8KB）
    http::request<http::dynamic_body> _request;  // 存储HTTP请求内容
    http::response<http::dynamic_body> _response; // 存储待发送的HTTP响应
    net::steady_timer _deadlineTimer{     // 连接超时计时器
        _socket.get_executor(),           // 使用socket的执行器
        std::chrono::seconds(60)          // 默认超时时间60秒
    };
    std::string _getURL;
    std::unordered_map<std::string, std::string> _getParams;
};