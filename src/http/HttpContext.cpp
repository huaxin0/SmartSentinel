#include "../../include/http/HttpContext.h"

using namespace muduo;
using namespace muduo::net;

namespace http
{

// 将报文解析出来将关键信息封装到HttpRequest对象里面去
bool HttpContext::parseRequest(Buffer *buf, Timestamp receiveTime)
{
    bool ok = true; // 解析每行请求格式是否正确
    bool hasMore = true;
    while (hasMore)
    {
        if (state_ == kExpectRequestLine)
        {
            const char *crlf = buf->findCRLF(); // 注意这个返回值边界可能有错
            if (crlf)
            {
                ok = processRequestLine(buf->peek(), crlf);
                if (ok)
                {
                    request_.setReceiveTime(receiveTime);
                    buf->retrieveUntil(crlf + 2);   // 下一行（Host）的开头,移动指针
                    state_ = kExpectHeaders;    // 2.【变身】状态切换：下一步准备读 Header
                }
                else
                {
                    hasMore = false;
                }
            }
            else
            {
                hasMore = false;
            }
        }
        else if (state_ == kExpectHeaders)
        {
            const char *crlf = buf->findCRLF(); //找这一行的结尾 (\r\n)
            if (crlf)   // 如果找到了回车换行，说明缓冲区里至少有一整行数据
            {
                const char *colon = std::find(buf->peek(), crlf, ':');
                if (colon < crlf)
                {   // 既然有冒号，就把 Key 和 Value 抠出来，存进 request_ 对象里
                    request_.addHeader(buf->peek(), colon, crlf);
                }
                else if (buf->peek() == crlf)
                { 
                    // 空行，结束Header
                    // 根据请求方法和Content-Length判断是否需要继续读取body
                    //// HTTP 协议规定：Header 和 Body 之间必须有一个空行。//状态切换
                    // 只有 POST 或 PUT 请求才会有 Body，GET 请求通常没有
                    if (request_.method() == HttpRequest::kPost || 
                        request_.method() == HttpRequest::kPut)
                    {
                        std::string contentLength = request_.getHeader("Content-Length");
                        // 看看 Header 里有没有 Content-Length
                        if (!contentLength.empty())
                        {   
                            // 把长度转成数字存起来 (比如 "100" -> 100)
                            request_.setContentLength(std::stoi(contentLength));
                            if (request_.contentLength() > 0)
                            {
                                state_ = kExpectBody;
                            }
                            else
                            {   // 长度是0，说明没数据，直接收工
                                state_ = kGotAll;
                                hasMore = false;
                            }
                        }
                        else
                        {
                            // POST/PUT 请求没有 Content-Length，是HTTP语法错误
                            ok = false;
                            hasMore = false;
                        }
                    }
                    else
                    {
                        // GET/HEAD/DELETE 等方法直接完成（没有请求体）
                        state_ = kGotAll; 
                        hasMore = false;
                    }
                }
                else
                {
                    ok = false; // Header行格式错误
                    hasMore = false;
                }

                buf->retrieveUntil(crlf + 2); // 开始读指针指向下一行数据
            }
            else
            {
                hasMore = false;
            }
        }
        else if (state_ == kExpectBody)
        {
            // 检查缓冲区中是否有足够的数据
            if (buf->readableBytes() < request_.contentLength())
            {
                hasMore = false; // 数据不完整，等待更多数据
                return true;
            }

            // 只读取 Content-Length 指定的长度
            std::string body(buf->peek(), buf->peek() + request_.contentLength());
            request_.setBody(body);

            // 准确移动读指针
            buf->retrieve(request_.contentLength());

            state_ = kGotAll;
            hasMore = false;
        }
    }
    return ok; // ok为false代表报文语法解析错误
}

// 解析请求行, 例子GET /home?id=1 HTTP/1.1  解析报文
bool HttpContext::processRequestLine(const char *begin, const char *end)
{
    bool succeed = false;
    const char *start = begin; //开始指针进入的是数据包的包头
    const char *space = std::find(start, end, ' '); //找到第一个空格
    // 3. 如果找到了空格，就把 start 到 space 之间的东西 ("GET") 拿去解析
    if (space != end && request_.setMethod(start, space))
    {
        start = space + 1;  //start 跳过空格，指向 '/'
        space = std::find(start, end, ' ');  //找到下一个空格
        if (space != end)
        {
            const char *argumentStart = std::find(start, space, '?'); //在这两点之间，找有没有 '?'
            if (argumentStart != space) // 请求带参数
            {   
                //这里的 start 是 '/'，argumentStart 是 '?'，space是空格
                request_.setPath(start, argumentStart); // 注意这些返回值边界,// 切出 "/home"
                request_.setQueryParameters(argumentStart + 1, space); // 切出 "id=1"
            }
            else // 请求不带参数 ， // 情况 B：没找到 '?' (比如只是 /home)，
            {
                request_.setPath(start, space); // 整个都是路径 
            }

            start = space + 1;  // 指针移到 'H' (HTTP的开头)
            succeed = ((end - start == 8) && std::equal(start, end - 1, "HTTP/1."));
            if (succeed)
            {
                if (*(end - 1) == '1')
                {
                    request_.setVersion("HTTP/1.1");
                }
                else if (*(end - 1) == '0')
                {
                    request_.setVersion("HTTP/1.0");
                }
                else
                {
                    succeed = false;
                }
            }
        }
    }
    return succeed;
}

} // namespace http