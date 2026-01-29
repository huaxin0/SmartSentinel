#include "../../include/http/HttpResponse.h"
#include <muduo/net/Buffer.h>
#include <stdio.h>

using namespace muduo;
using namespace muduo::net;

namespace http
{

void HttpResponse::appendToBuffer(muduo::net::Buffer* output) const
{
    // 1. 响应行
    char buf[32];
    snprintf(buf, sizeof buf, "HTTP/1.1 %d ", statusCode_);
    output->append(buf);
    output->append(statusMessage_);
    output->append("\r\n");

    // 2. 自动添加 Content-Length / Connection
    if (closeConnection_)
    {
        output->append("Connection: close\r\n");
    }
    else
    {
        snprintf(buf, sizeof buf, "Content-Length: %zd\r\n", body_.size());
        output->append(buf);
        output->append("Connection: Keep-Alive\r\n");
    }

    // 3. 遍历添加其他头部 (CORS 头等)
    for (const auto& header : headers_)
    {
        output->append(header.first);
        output->append(": ");
        output->append(header.second);
        output->append("\r\n");
    }

    // 4. 头部结束空行
    output->append("\r\n");

    // 5. 响应体
    output->append(body_);
}

void HttpResponse::setStatusLine(const std::string& version,
                                 HttpStatusCode statusCode,
                                 const std::string& statusMessage)
{
    httpVersion_ = version;
    statusCode_ = statusCode;
    statusMessage_ = statusMessage;
}

} // namespace http