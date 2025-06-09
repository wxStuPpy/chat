#include "HttpConnection.hpp"

HttpConnection::HttpConnection(tcp::socket socket)
    : _socket(std::move(socket))
{

}
void HttpConnection::start(){
    // 获取当前对象的智能指针
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
    _deadlineTimer.async_wait(
        [self](beast::error_code ec){
            if(!ec)
                self->_socket.close();
        });
}

void HttpConnection::writeResponse(){
    auto self(shared_from_this());
    _response.content_length(_response.body().size());
    http::async_write(_socket,_response, 
        [self](beast::error_code ec, std::size_t bytes_transferred)
        {
            self->_socket.shutdown(tcp::socket::shutdown_send, ec);
            self->_deadlineTimer.cancel();
        });
}

void HttpConnection::handleReq(){
    //设置版本
    _response.version(_request.version());
    _response.keep_alive(false);
    if(_request.method() == http::verb::get){
      bool success=LogicSystem::getInstance().handleGet(_request.target(), shared_from_this());
      if(!success){
        _response.result(http::status::not_found);
        _response.set(http::field::content_type, "text/plain");
        beast::ostream(_response.body()) << "URL Not Found\r\n";
        writeResponse();
        return;
      }

      _response.result(http::status::ok);
      _response.set(http::field::server, "gateServer");
      writeResponse();
      return;
    }
}
