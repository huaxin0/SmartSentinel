#include "http/HttpServer.h"
#include "http/HttpContext.h"
#include "http/HttpRequest.h"
#include "http/HttpResponse.h"

using namespace muduo;
using namespace muduo::net;

namespace http
{

// 默认回调：如果你没设置回调，就返回 404
void defaultHttpCallback(const HttpRequest&, HttpResponse* resp)
{
    resp->setStatusCode(HttpResponse::k404NotFound);
    resp->setStatusMessage("Not Found");
    resp->setCloseConnection(true);
}

HttpServer::HttpServer(EventLoop* loop,
                       const InetAddress& listenAddr,
                       const std::string& name,
                       TcpServer::Option option)
  : server_(loop, listenAddr, name, option),
    httpCallback_(defaultHttpCallback)
{
    server_.setConnectionCallback(
        std::bind(&HttpServer::onConnection, this, _1));
    server_.setMessageCallback(
        std::bind(&HttpServer::onMessage, this, _1, _2, _3));
}

HttpServer::~HttpServer()
{
}

void HttpServer::start()
{
    LOG_INFO << "HttpServer[" << server_.name() << "] starts listening on " << server_.ipPort();
    server_.start();
}

void HttpServer::onConnection(const TcpConnectionPtr& conn)
{
    if (conn->connected())
    {
        // 连接建立时，绑定一个 HttpContext 到这个连接上
        // 这样每个连接都有自己独立的解析上下文
        conn->setContext(HttpContext());
    }
}

void HttpServer::onMessage(const TcpConnectionPtr& conn,
                           Buffer* buf,
                           Timestamp receiveTime)
{
    // 取出当前连接的上下文
    HttpContext* context = boost::any_cast<HttpContext>(conn->getMutableContext());

    // 解析请求
    if (!context->parseRequest(buf, receiveTime))
    {
        // 解析出错，直接发 400 错误并关闭连接
        conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");
        conn->shutdown();
    }

    // 如果解析完成了一个完整的请求
    if (context->gotAll())
    {
        // 处理请求
        onRequest(conn, context->request());
        // 重置上下文，准备接收下一个请求 (Keep-Alive)
        context->reset();
    }
}

void HttpServer::onRequest(const TcpConnectionPtr& conn, const HttpRequest& req)
{
    const string& connection = req.getHeader("Connection");
    bool close = (connection == "close") ||
                 (req.getVersion() == "HTTP/1.0" && connection != "Keep-Alive");

    // 构造响应对象
    HttpResponse response(close);

    // 【关键】调用你在 main.cpp 里设置的 dispatch 函数
    if (httpCallback_)
    {
        httpCallback_(req, &response);
    }

    // 发送响应数据
    Buffer buf;
    response.appendToBuffer(&buf);
    conn->send(&buf);

    // 如果是短连接，发完就关
    if (response.closeConnection())
    {
        conn->shutdown();
    }
}

} // namespace http