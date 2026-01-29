#include "../../include/controller/UserController.h" //接口
#include "../../include/db/DbConnectionPool.h"  //引入数据库连接池
#include "../../src/base/json.hpp"  //引入json格式
#include <iostream>
#include <string>
using json=nlohmann::json;
using namespace http;
using namespace http::db;

// ==========================================================
// 核心功能：登录接口实现
// ==========================================================
void UserController::login(const HttpRequest& req, HttpResponse* resp) {

    // ==========================================================
    // NEW: 添加 CORS 跨域头 (给浏览器的通行证)
    // ==========================================================
    resp->addHeader("Access-Control-Allow-Origin", "*");
    resp->addHeader("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
    resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");

    // ==========================================================
    // NEW: 处理浏览器的“试探”请求 (OPTIONS)
    // ==========================================================
    // 注意：这里假设 HttpRequest::kOptions 是你的枚举值。
    // 如果你的 method() 返回的是 string，请改成 if (req.methodString() == "OPTIONS")
    if (req.method() == HttpRequest::kOptions) {
        resp->setStatusCode(HttpResponse::k200Ok);
        resp->setStatusMessage("OK"); // <--- 【一定要加上这一句】
        return; 
    }

    // ------------------------------------------------------
    // STEP 1: 协议设置
    // ------------------------------------------------------
    // 告诉浏览器：无论成功失败，我返回的都是 JSON 格式的数据
    resp->addHeader("Content-Type","application/json");
    // ------------------------------------------------------
    // STEP 2: 解析与安检 (Parsing)
    // ------------------------------------------------------
    std::string body=req.getBody(); //获取Http包体
    json reqJson; //创建一个空的JSON对象
    try {
        reqJson=json::parse(body);
    }catch(...){
        resp->setStatusCode(HttpResponse::k400BadRequest);
        resp->setBody(R"({"code":400,"msg":"Invalid JSON format"})");
        return;
    }
    // 从 JSON 中提取字段
    // .value("key", default_value) 是一种安全的获取方式
    // 如果前端没传 "username"，我们就拿到空字符串 ""
    std::string username = reqJson.value("username", "");
    std::string password = reqJson.value("password", "");
    // 简单的参数校验
    if(username.empty()||password.empty()){
        resp->setStatusCode(HttpResponse::k400BadRequest);
        resp->setBody(R"({"code":400,"msg":"Username or password cannot be empty"})");
        return;
    }
    // ------------------------------------------------------
    // STEP 3: 获取数据库资源 (Resource)
    // ------------------------------------------------------
    // 从单例连接池中“借”一个连接
    // getConnection() 返回的是 shared_ptr，用完出作用域会自动归还，不用担心内存泄漏
    auto conn=DbConnectionPool::getInstance().getConnection();
    /*  等价于 第一步先拿到连接池的大管家（对象引用）
        DbConnectionPool& pool = DbConnectionPool::getInstance();
        //    第二步：再问大管家要一个连接
        auto conn = pool.getConnection(); */
    if(!conn){
        resp->setStatusCode(HttpResponse::k500InternalServerError);
        resp->setBody(R"({"code":500,"msg":"Database connection unavailable"})");
        return;
    }
    // ------------------------------------------------------
    // STEP 4: 数据库查询 (Business Logic)
    // ------------------------------------------------------
    // 准备 SQL 语句。
    // 注意：我们使用 ? 作为占位符，而不是拼接字符串。
    // 这样如果用户输入 "admin' OR '1'='1"，会被当成纯文本处理，防止 SQL 注入攻击。
    std::string sql = "SELECT id, username FROM users WHERE username = ? AND password = ?";
    // 执行查询，传入参数。conn 会自动帮我们把 username 和 password 填到 ? 的位置
    auto result = conn->executeQuery(sql, username, password);
    // ------------------------------------------------------
    // STEP 5: 构造响应 (Response)
    // ------------------------------------------------------
    json respJson; // 准备返回给前端的 JSON
    // result->next() 返回 true 说明查到了数据（也就是账号密码匹配）
    if (result && result->next()) {
        // --- 登录成功 ---
        int userId = result->getInt("id");
        
        resp->setStatusCode(HttpResponse::k200Ok);
        resp->setStatusMessage("OK");
        // 构造标准的 API 返回结构
        respJson["code"] = 0; // 0 表示业务成功
        respJson["msg"] = "Login Success";
        
        // 返回一些业务数据
        respJson["data"] = {
            {"userId", userId},
            {"username", username},
            // 在这里，通常我们会生成一个 JWT Token 返回给前端
            // 前端下次请求带着这个 Token，我们就知道他是谁了
            // 目前先返回一个假的 Token 占位
            {"token", "mock_token_xyz_123"} 
        };
        
        std::cout << "[INFO] User login success: " << username << std::endl;
    } else {
        // --- 登录失败 ---
        // 虽然业务失败了，但 HTTP 状态码可以用 200 (表示服务器处理完了请求)
        // 也可以用 401 (Unauthorized)，这里我们用 401 更符合语义
        resp->setStatusCode(HttpResponse::k401Unauthorized);
        
        respJson["code"] = 1001; // 自定义错误码：1001 代表账号密码错误
        respJson["msg"] = "Username or password incorrect";
        
        std::cout << "[WARN] User login failed: " << username << std::endl;
    }

    // 最后，把 JSON 对象转成字符串 (.dump())，放入响应体
    resp->setBody(respJson.dump());
    
}

void UserController::registerUser(const HttpRequest& req, HttpResponse* resp) {
    // 提示：
    // 1. 解析 JSON
    // 2. 检查用户名是否已存在 (SELECT count(*) ...)
    // 3. 执行插入 (conn->executeUpdate("INSERT INTO ..."))
    // 4. 返回结果
}