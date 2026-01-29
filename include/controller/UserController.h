#pragma once // 防止头文件被重复包含
//引入http 请求和响应的定义
#include "../http/HttpRequest.h"
#include "../http/HttpResponse.h"
#include <string>

// UserController 类：专门负责处理和用户相关的业务逻辑
class UserController{
public:
    UserController()=default;
    ~UserController()=default;
    /**
     * @brief 处理用户登录请求
     * @param req  HTTP 请求对象（包含前端发来的账号密码）
     * @param resp HTTP 响应对象（我们要把结果写到这里面）
     * * 设计为 void 类型，因为结果通过修改 resp 指针来返回
     */
    void login(const http::HttpRequest& req, http::HttpResponse *resp);
    /**
     * @brief 处理用户注册请求
     * (这个你可以先声明在这里，等登录写完了再练手实现)
     */
    void registerUser(const http::HttpRequest& req, http::HttpResponse *resp);


};
