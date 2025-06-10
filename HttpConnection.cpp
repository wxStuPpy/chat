#include "HttpConnection.hpp"
#include "LogicSystem.hpp"

HttpConnection::HttpConnection(tcp::socket socket)
    : _socket(std::move(socket))
{

}
void HttpConnection::start(){
    
    auto self(shared_from_this());

    // 异步读取HTTP请求
    http::async_read(
        _socket,     // 套接字
        _buffer,     // 缓冲区
        self->_request, // 请求对象
        [self](beast::error_code ec, std::size_t bytes_transferred){
            try
            {
                // 检查是否有错误发生
                if(ec){
                    // 输出错误信息
                    std::cerr<<"http read error: "<<ec.message()<<"\n";
                    return;
                }

                // 忽略传输的字节数
                boost::ignore_unused(bytes_transferred);

                // 处理请求
                self->handleReq();

                // 检查截止日期
                self->checkDeadline();
            }
            catch(const std::exception& e)
            {
                // 输出异常信息
                std::cerr << e.what() << '\n';
            }
            
        });
}


void HttpConnection::checkDeadline(){
    auto self(shared_from_this());

    // 设置定时器异步等待
    _deadlineTimer.async_wait(
        [self](beast::error_code ec){
            // 如果定时器没有出错
            if(!ec)
                // 关闭套接字
                self->_socket.close();
        });
}

void HttpConnection::writeResponse(){
    auto self(shared_from_this());
    // 设置响应体的内容长度
    _response.content_length(_response.body().size());

    // 异步写入响应到socket 当写入操作完成或出错时，回调函数将被调用。
    http::async_write(_socket,_response, 
        [self](beast::error_code ec, std::size_t bytes_transferred)
        {
            // 关闭socket的发送功能(关闭发送功端) 
            self->_socket.shutdown(tcp::socket::shutdown_send, ec);
            // 取消计时器
            self->_deadlineTimer.cancel();
        });
}

void HttpConnection::handleReq(){
    // 设置响应的版本与请求的版本一致
    _response.version(_request.version());
    _response.keep_alive(false);

    if(_request.method() == http::verb::get){
        // 处理GET请求
        // 调用LogicSystem实例的handleGet方法处理GET请求
      bool success=LogicSystem::getInstance()->handleGet(_request.target().to_string(),shared_from_this());
        // 如果处理失败
      if(!success){
            // 设置响应状态码为未找到
        _response.result(http::status::not_found);
            // 设置响应头中的内容类型为纯文本
        _response.set(http::field::content_type, "text/plain");
            // 向响应体写入"URL Not Found"
        beast::ostream(_response.body()) << "URL Not Found\r\n";
            // 写入响应
        writeResponse();
        return;
      }

        // 如果处理成功
      _response.result(http::status::ok);
        // 设置响应头中的服务器名称
      _response.set(http::field::server, "gateServer");
        // 写入响应
      writeResponse();
      return;
    }
}
