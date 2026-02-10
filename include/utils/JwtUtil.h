#pragma once
#include "../jwt-cpp/jwt.h"
#include <string>
#include <chrono>
#include <iostream>
namespace utils{
class JwtUtil
{
private:
    // ==========================================================
    // 密钥 (Secret Key)
    // ==========================================================
    // 这是服务器最重要的东西！绝对不能泄露给客户端。
    // 只有拥有这个密钥，才能生成合法的签名，或者验证签名是否正确。
    // 在实际生产中，这个通常从环境变量或配置文件里读，不要硬编码。    
    static const std::string SECRET_KEY;
public:
    // ==========================================================
    // 生成 Token (Create Token)
    // ==========================================================
    static std::string createToken(int userId,const::string& username){
        // 获取当前时间
        auto now =std::chrono::system_clock::now();
        //使用jwt-cpp的构建器，builder
        auto token =jwt::create()
            // 1. 设置发行者 (Issuer)，由谁签发
            .set_issuer("auto0")
            // 2. 设置类型 (Type)，通常是 JWS
            .set_type("JWS")
            // 3. 设置载荷 (Payload) - 放入我们要存的用户数据
            // 注意：不要放密码等敏感信息！
            .set_payload_claim("userId",jwt::claim(std::to_string(userId)))
            .set_payload_claim("username",jwt::claim(username))
            // 4. 设置时间
            .set_issued_at(now) //签发时间
            .set_expires_at(now+std::chrono::hours(24)) //24h过期
            // 5. 签名 (Sign)
            // 使用 HS256 算法，配合密钥进行加密
            .sign(jwt::algorithm::hs256{SECRET_KEY});
        return token;  //类型是string
    }
    // ==========================================================
    // 验证 Token (Verify Token)
    // ==========================================================
    // 返回值：如果成功，返回 userId；如果失败（过期、篡改），返回 -1
    static int verifyToken(const std::string& token){
        if(token.empty()) return -1;
        try{
            // 1. 创建解码器 (Verifier)
            // 告诉它，我们要用 HS256 算法和那个密钥来验证
            auto verifer= jwt::verify()
                .allow_algorithm(jwt::algoritm::hs256{SECRET_KEY})
                .with_issuer("auth0");// 检查发行者是不是我们需要的那个人

            // 2. 解析 Token 字符串
            auto decoded =jwt::decode(token);
            // 3. 执行验证
            // 这一步会检查：
            // - 签名对不对？(防止篡改)
            // - 时间有没有过期？(Expires At)
            // 如果有问题，这里会直接抛出异常 (throw exception)
            verifier.verify(decoded);
            // 4. 如果没抛异常，说明 Token 是真的。
            // 我们把里面的 userId 取出来
            if (decoded.has_payload_claim("userId")) {
                // 注意：我们在 createToken 里存的是 string，这里取出来转回 int
                std::string userIdStr = decoded.get_payload_claim("userId").as_string();
                return std::stoi(userIdStr);  //转回int类型的数据
            }
            return -1; // 也就是没有 userId 字段    
        }catch (const std::exception& e) {
            // 捕获所有错误：比如 Token 过期、签名不对、格式烂了等等
            std::cerr << "[JWT Error] Verify failed: " << e.what() << std::endl;
            return -1;
        }

    }  
};

const std::string JwtUtil::SECRET_KEY = "SmartSentinel_Secret_Key_2026";


}
