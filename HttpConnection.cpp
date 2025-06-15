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

unsigned char toHex(unsigned char x){
    return x > 9 ? x - 10 + 'a' : x + '0';
}

unsigned char fromHex(unsigned char x)
{
    unsigned char y;
    if (x >= 'A' && x <= 'Z') y = x - 'A' + 10;
    else if (x >= 'a' && x <= 'z') y = x - 'a' + 10;
    else if (x >= '0' && x <= '9') y = x - '0';
    else assert(0);
    return y;
}

std::string urlEncode(const std::string& str)
{
    std::string strTemp = "";
    size_t length = str.length();
    for (size_t i = 0; i < length; i++)
    {
        //判断是否仅有数字和字母构成
        if (isalnum((unsigned char)str[i]) ||
            (str[i] == '-') ||
            (str[i] == '_') ||
            (str[i] == '.') ||
            (str[i] == '~'))
            strTemp += str[i];
        else if (str[i] == ' ') //为空字符
            strTemp += "+";
        else
        {
            //其他字符需要提前加%并且高四位和低四位分别转为16进制
            strTemp += '%';
            strTemp += toHex((unsigned char)str[i] >> 4);
            strTemp += toHex((unsigned char)str[i] & 0x0F);
        }
    }
    return strTemp;
}

std::string urlDecode(const std::string& str)
{
    std::string strTemp = "";
    size_t length = str.length();
    for (size_t i = 0; i < length; i++)
    {
        //还原+为空
        if (str[i] == '+') strTemp += ' ';
        //遇到%将后面的两个字符从16进制转为char再拼接
        else if (str[i] == '%')
        {
            assert(i + 2 < length);
            unsigned char high = fromHex((unsigned char)str[++i]);
            unsigned char low = fromHex((unsigned char)str[++i]);
            strTemp += high * 16 + low;
        }
        else strTemp += str[i];
    }
    return strTemp;
}

void HttpConnection::preParseGetParam() {
    // 提取 URI  
    auto uri = _request.target();
    // 查找查询字符串的开始位置（即 '?' 的位置）  
    auto query_pos = uri.find('?');
    if (query_pos == std::string::npos) {
        _getURL = uri.to_string();
        return;
    }
    _getURL = uri.to_string().substr(0, query_pos);
    std::string query_string = uri.to_string().substr(query_pos + 1);
    std::string key;
    std::string value;
    size_t pos = 0;
    while ((pos = query_string.find('&')) != std::string::npos) {
        auto pair = query_string.substr(0, pos);
        size_t eq_pos = pair.find('=');
        if (eq_pos != std::string::npos) {
            key = urlDecode(pair.substr(0, eq_pos)); // 假设有 url_decode 函数来处理URL解码  
            value = urlDecode(pair.substr(eq_pos + 1));
            _getParams[key] = value;
        }
        query_string.erase(0, pos + 1);
    }
    // 处理最后一个参数对（如果没有 & 分隔符）  
    if (!query_string.empty()) {
        size_t eq_pos = query_string.find('=');
        if (eq_pos != std::string::npos) {
            key = urlDecode(query_string.substr(0, eq_pos));
            value = urlDecode(query_string.substr(eq_pos + 1));
            _getParams[key] = value;
        }
    }
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
