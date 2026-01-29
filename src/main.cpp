#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>
#include <muduo/base/Logging.h>

#include "http/HttpServer.h"
#include "http/HttpRequest.h"
#include "http/HttpResponse.h"
#include "controller/UserController.h"
#include "db/DbConnectionPool.h"

#include <functional>
#include <map>
#include <string>

using namespace muduo;
using namespace muduo::net;
using namespace http;

// 定义路由处理函数的类型
using HttpHandler = std::function<void(const HttpRequest&, HttpResponse*)>;

// 全局路由表：把 URL 字符串映射到具体的处理函数
// 例如："/api/user/login" -> UserController::login
std::map<std::string, HttpHandler> g_router;

// ----------------------------------------------------------
// 核心分发器：HttpServer 收到请求后会调用这个函数
// ----------------------------------------------------------
void dispatchRequest(const HttpRequest& req, HttpResponse* resp)
{
    std::string path = req.path();
    
    // 打印日志，方便调试
    LOG_INFO << "Received Request: " << req.method() << " " << path;

    // 在路由表里查找有没有对应的处理器
    auto it = g_router.find(path);
    if (it != g_router.end())
    {
        // 找到了！调用对应的函数（比如 UserController::login）
        it->second(req, resp);
    }
    else
    {
        // 没找到，返回 404
        resp->setStatusCode(HttpResponse::k404NotFound);
        resp->setStatusMessage("Not Found");
        resp->setBody("404 Not Found: " + path);
        resp->setCloseConnection(true);
    }
}

int main(int argc, char* argv[])
{
    // 设置日志级别 (INFO)
    Logger::setLogLevel(Logger::INFO);

    LOG_INFO << "Starting SmartSentinel Server...";

    // ------------------------------------------------------
    // 1. 初始化数据库连接池
    // ------------------------------------------------------
    // 注意：请确保你的 MySQL 容器正在运行，且密码是 123456
    // 参数顺序通常是: host, user, password, dbname, port, poolSize
    // 如果你的 init 函数参数不一样，请根据 DbConnectionPool.h 修改这里
    try {
        db::DbConnectionPool::getInstance().init(
            "127.0.0.1",       // 数据库IP (因为用了 --network host)
            "root",            // 用户名
            "123456",          // 密码
            "smart_sentinel_db", // 数据库名
            10                 // 连接池大小
        );
        LOG_INFO << "Database initialized successfully.";
    } catch (const std::exception& e) {
        LOG_FATAL << "Database init failed: " << e.what();
    }

    // ------------------------------------------------------
    // 2. 初始化 Controller (业务逻辑控制器)
    // ------------------------------------------------------
    UserController userController;

    // ------------------------------------------------------
    // 3. 注册路由 (手动把 URL 和函数绑定起来)
    // ------------------------------------------------------
    // 绑定 /api/user/login 到 userController.login
    g_router["/api/user/login"] = std::bind(&UserController::login, &userController, std::placeholders::_1, std::placeholders::_2);
    
    // 如果你写了注册功能，可以在这里解开注释
    // g_router["/api/user/register"] = std::bind(&UserController::registerUser, &userController, _1, _2);

    // ------------------------------------------------------
    // 4. 启动 HTTP 服务器
    // ------------------------------------------------------
    EventLoop loop;
    InetAddress addr(8083); // 监听 8083 端口
    HttpServer server(&loop, addr, "SmartSentinel");

    // 设置回调函数为上面的 dispatchRequest
    server.setHttpCallback(dispatchRequest);
    
    // 设置线程数 (根据你的 CPU 核心数调整，0 表示只有主线程)
    server.setThreadNum(4); 

    server.start();
    
    LOG_INFO << "Server is running on port 8083...";
    
    // 进入事件循环 (死循环，直到程序退出)
    loop.loop();

    return 0;
}