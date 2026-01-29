#pragma once 

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/base/Logging.h>
#include <muduo/net/InetAddress.h>

#include <string>
#include <functional>

// 只需要引用这两个，不需要 Router 那些
#include "HttpRequest.h"
#include "HttpResponse.h"

namespace http
{

class HttpServer : muduo::noncopyable
{
public:
    // 定义回调函数类型：当收到完整的 HTTP 请求时调用
    using HttpCallback = std::function<void (const HttpRequest&, HttpResponse*)>;
    
    // 构造函数
    HttpServer(muduo::net::EventLoop* loop,
               const muduo::net::InetAddress& listenAddr,
               const std::string& name,
               muduo::net::TcpServer::Option option = muduo::net::TcpServer::kNoReusePort);
    
    // 析构函数
    ~HttpServer(); 

    // 设置处理 HTTP 请求的回调函数 (对应 main.cpp 里的 dispatch)
    void setHttpCallback(const HttpCallback& cb)
    {
        httpCallback_ = cb;
    }

    // 设置线程数
    void setThreadNum(int numThreads)
    {
        server_.setThreadNum(numThreads);
    }

    // 启动服务器
    void start();

private:
    // Muduo TcpServer 的连接回调
    void onConnection(const muduo::net::TcpConnectionPtr& conn);
    
    // Muduo TcpServer 的消息回调
    void onMessage(const muduo::net::TcpConnectionPtr& conn,
                   muduo::net::Buffer* buf,
                   muduo::Timestamp receiveTime);
                   
    // 内部处理请求的函数
    void onRequest(const muduo::net::TcpConnectionPtr&, const HttpRequest&);

private:
    muduo::net::TcpServer server_;
    HttpCallback httpCallback_; // 保存 main.cpp 传进来的 dispatch 函数
}; 

} // namespace http