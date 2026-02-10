#pragma once
#include "../http/HttpRequest.h"
#include "../http/HttpResponse.h"

class  DeviceController
{
public:
    // 设备控制器：专门处理和 推理 / 摄像头 有关的请求
    // 获取设备列表 (前端调用，用来显示这人名下有哪些摄像头)
    void listDevices(const http::HttpRequest& req, http::HttpResponse* resp);
    // 注册设备 (边缘端调用，用来告诉云端“我上线了”)
    void registerDevice(const http::HttpRequest& req,http::HttpResponse* resp);
};
