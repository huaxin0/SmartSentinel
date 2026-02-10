# 1. 版本要求
cmake_minimum_required(VERSION 3.10)

# 2. 项目名称
project(SmartSentinel)

# 3. C++ 标准 (Muduo 和现代 C++ 通常需要 C++11 或以上，这里用 C++14)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 4. 设置编译选项 (开启调试信息 -g，开启所有警告 -Wall)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall")

# 5. 设置最终可执行文件的输出路径 -> SmartSentinel/bin/
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# =============================================================
# 核心配置：头文件路径 (Include Directories)
# =============================================================
# 告诉编译器去哪里找 .h 文件
# 这样你在代码里写 #include "http/HttpServer.h" 时
# 编译器会自动去 SmartSentinel/include/http/HttpServer.h 找
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    /usr/include/mysql      # MySQL 头文件的系统路径
    /usr/local/include      # 假如 Muduo 装在这里
)

# =============================================================
# 核心配置：源文件查找 (Source Files)
# =============================================================
# 递归扫描 src 目录下所有的 .cpp 文件，存入变量 SRC_FILES
file(GLOB_RECURSE SRC_FILES "src/*.cpp")

# =============================================================
# 核心配置：库文件查找 (Libraries)
# =============================================================
# 查找线程库 (Muduo 需要)
find_package(Threads REQUIRED)

# =============================================================
# 生成目标
# =============================================================
add_executable(SmartSentinel ${SRC_FILES})

# =============================================================
# 链接库 (Linking)
# =============================================================
# 告诉编译器，链接这些具体的库文件
target_link_libraries(SmartSentinel
    muduo_net           # Muduo 网络库
    muduo_base          # Muduo 基础库
    mysqlclient         # MySQL 官方 C 客户端库 (C++ Connector底层也依赖它)
    mysqlcppconn        # MySQL C++ Connector 库
    pthread             # 线程库
    # opencv_core       # 未来做视频时再解开注释
    # opencv_highgui    # ...
)