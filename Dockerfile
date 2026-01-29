# 1. 直接使用本地已有的镜像作为地基
FROM nvcr.io/nvidia/tensorrt:22.12-py3

# 2. 换源 
RUN sed -i 's/archive.ubuntu.com/mirrors.aliyun.com/g' /etc/apt/sources.list && \
    sed -i 's/security.ubuntu.com/mirrors.aliyun.com/g' /etc/apt/sources.list

# 3. 安装 C++ 开发全家桶
# 包含：编译器、CMake、调试器、MySQL连接库、SSL(HTTPS)、OpenCV
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    gdb \
    vim \
    libmysqlclient-dev \
    libssl-dev \
    libopencv-dev \
    pkg-config \
    net-tools \
    iputils-ping \
    && rm -rf /var/lib/apt/lists/*

# 4. 设置工作目录
WORKDIR /app

# 5. 默认进入 bash
CMD ["/bin/bash"]