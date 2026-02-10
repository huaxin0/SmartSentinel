#include "../../include/db/DbConnectionPool.h"
#include "../../include/db/DbException.h"
#include <muduo/base/Logging.h>

namespace http 
{
namespace db 
{

void DbConnectionPool::init(const std::string& host,
                          const std::string& user,
                          const std::string& password,
                          const std::string& database,
                          size_t poolSize) 
{
    // 连接池会被多个线程访问，所以操作其成员变量时需要加锁
    std::lock_guard<std::mutex> lock(mutex_);
    // 确保只初始化一次
    if (initialized_) 
    {
        return;
    }

    host_ = host;
    user_ = user;
    password_ = password;
    database_ = database;

    // 创建连接
    for (size_t i = 0; i < poolSize; ++i) 
    {
        connections_.push(createConnection());
    }

    initialized_ = true;
    LOG_INFO << "Database connection pool initialized with " << poolSize << " connections";
}

DbConnectionPool::DbConnectionPool() 
{
    checkThread_ = std::thread(&DbConnectionPool::checkConnections, this);
    checkThread_.detach();
}

DbConnectionPool::~DbConnectionPool() 
{
    std::lock_guard<std::mutex> lock(mutex_);
    while (!connections_.empty()) 
    {
        connections_.pop();
    }
    LOG_INFO << "Database connection pool destroyed";
}

// 修改获取连接的函数
std::shared_ptr<DbConnection> DbConnectionPool::getConnection() 
{
    std::shared_ptr<DbConnection> conn;
    {
        std::unique_lock<std::mutex> lock(mutex_); //加锁
        
        while (connections_.empty())  //连接池空了就等待
        {
            if (!initialized_) 
            {
                throw DbException("Connection pool not initialized");
            }
            LOG_INFO << "Waiting for available connection...";
            cv_.wait(lock);  //等待条件变量通知,（释放锁+挂起线程）
        }
        
        conn = connections_.front(); //取出队首连接
        connections_.pop();    //弹出队首连接
    } // 释放锁
    
    try 
    {
        // 在锁外检查连接
        if (!conn->ping())  //连接断了返回 false就是0， 连接成功返回 true就是1
        {
            LOG_WARN << "Connection lost, attempting to reconnect...";
            conn->reconnect(); //重新连接数据库
        }
        // 8. 重新包装一个 shared_ptr 返回给用户, 并自定义删除器，归还连接到连接池
        return std::shared_ptr<DbConnection>(conn.get(), 
            [this, conn](DbConnection*) {
                std::lock_guard<std::mutex> lock(mutex_);  // 上锁
                connections_.push(conn);  // 把连接放回队列
                cv_.notify_one();  // 通知等待的线程有连接可用
            });
    } 
    catch (const std::exception& e) //捕获所有异常
    {
        LOG_ERROR << "Failed to get connection: " << e.what(); //记录错误日志
        {
            std::lock_guard<std::mutex> lock(mutex_);
            connections_.push(conn);
            cv_.notify_one();
        }
        throw;
    }
}

std::shared_ptr<DbConnection> DbConnectionPool::createConnection() 
{
    return std::make_shared<DbConnection>(host_, user_, password_, database_);
}

// 修改检查连接的函数
void DbConnectionPool::checkConnections() 
{
    while (true) 
    {
        try 
        {
            std::vector<std::shared_ptr<DbConnection>> connsToCheck;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                if (connections_.empty()) 
                {
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                    continue;
                }
                
                auto temp = connections_;
                while (!temp.empty()) 
                {
                    connsToCheck.push_back(temp.front());
                    temp.pop();
                }
            }
            
            // 在锁外检查连接
            for (auto& conn : connsToCheck) 
            {
                if (!conn->ping()) 
                {
                    try 
                    {
                        conn->reconnect();
                    } 
                    catch (const std::exception& e) 
                    {
                        LOG_ERROR << "Failed to reconnect: " << e.what();
                    }
                }
            }
            
            std::this_thread::sleep_for(std::chrono::seconds(60));
        } 
        catch (const std::exception& e) 
        {
            LOG_ERROR << "Error in check thread: " << e.what();
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
}

} // namespace db
} // namespace http