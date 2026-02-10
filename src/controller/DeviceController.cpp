#include "../../include/controller/DeviceController.h"
#include "../../include/db/DbConnectionPool.h"
#include "../../include/utils/JwtUtil.h"
#include "../../src/base/json.hpp"
#include <iostream>

using json = nlohmann::json;
using namespace http;
using namespace http::db;

// ==========================================================
// 接口：获取设备列表
// URL: GET /api/device/list
// Header: Authorization: <token>
// ==========================================================
void DeviceController::listDevices(const HttpRequest& req, HttpResponse* resp) {
    
    // 1. 设置跨域头 (标准操作)
    resp->addHeader("Access-Control-Allow-Origin", "*");
    resp->addHeader("Content-Type", "application/json");

    // 2. 处理 OPTIONS 请求 (浏览器预检)
    if (req.method() == HttpRequest::kOptions) {
        return;
    }

    // ==========================================================
    // 3. 鉴权 (Security Check)
    // ==========================================================
    // 从 Header 中取出 Token。通常前端传过来是 "Authorization: token_string"
    std::string token = req.getHeader("Authorization");
    
    // 调用工具类验证 Token
    int userId = utils::JwtUtil::verifyToken(token);
    
    if (userId == -1) {
        // 如果验证失败，直接返回 401，不往下执行了
        resp->setStatusCode(HttpResponse::k401Unauthorized);
        resp->setBody(R"({"code":401, "msg":"Invalid Token"})");
        return;
    }
    std::cout << "[INFO] Valid request from userId: " << userId << std::endl;
    // ==========================================================
    // 4. 业务逻辑：查询该用户的设备
    // ==========================================================
    auto conn = DbConnectionPool::getInstance().getConnection();
    if (!conn) {
        resp->setStatusCode(HttpResponse::k500InternalServerError);
        resp->setBody(R"({"code":500, "msg":"DB Error"})");
        return;
    }

    // SQL: 这里的 ? 会被 userId 替换，查找这个用户所有的设备
    std::string sql = "SELECT id, device_name, rtsp_url, status FROM devices WHERE user_id = ?";
    auto result = conn->executeQuery(sql, userId);

    json deviceList = json::array(); // 创建一个 JSON 数组
    while (result && result->next()) {
        json item;
        item["id"] = result->getInt("id");
        item["name"] = result->getString("device_name");
        item["rtsp"] = result->getString("rtsp_url");
        item["status"] = result->getInt("status") == 1 ? "Online" : "Offline";
        deviceList.push_back(item);
    }

    // 5. 返回结果
    json respJson;
    respJson["code"] = 0;
    respJson["msg"] = "Success";
    respJson["data"] = deviceList;

    resp->setBody(respJson.dump());
}


void DeviceController::registerDevice(const HttpRequest& req, HttpResponse* resp) {
    // 这里的逻辑通常是边缘端(2080Ti)开机时调用的
    // 边缘端会发送 {"uuid": "xx", "ip": "xx", "userId": 1}
    
    // ... (鉴权逻辑同上，或者设备使用特殊的 API Key，这里简化跳过) ...
    json reqJson;
    try {
        reqJson = json::parse(req.getBody());
    } catch (...) {
        // JSON 格式不对
        return; 
    }

    std::string uuid = reqJson.value("uuid", "");
    std::string ip = reqJson.value("ip", "");
    int userId = reqJson.value("userId", 0); // 暂时先假设设备端知道属于谁

    // 数据库操作：如果有这个 UUID 就更新 IP 和状态，没有就插入
    // "INSERT ... ON DUPLICATE KEY UPDATE ..." 是 MySQL 的神技
    std::string sql = "INSERT INTO devices (user_id, device_name, device_uuid, device_ip, status) "
                      "VALUES (?, 'New Device', ?, ?, 1) "
                      "ON DUPLICATE KEY UPDATE device_ip = ?, status = 1";
    
    auto conn = DbConnectionPool::getInstance().getConnection();
    // 这里的参数顺序要严格对应 SQL 中的 ?
    conn->executeUpdate(sql, userId, uuid, ip, ip); 

    json respJson;
    respJson["code"] = 0;
    respJson["msg"] = "Device Registered";
    resp->setBody(respJson.dump());
}