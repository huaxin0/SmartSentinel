-- 1. 选中数据库
USE smart_sentinel_db;

-- 2. 创建用户表 users
CREATE TABLE IF NOT EXISTS users (
    id INT AUTO_INCREMENT PRIMARY KEY,
    username VARCHAR(50) NOT NULL UNIQUE,
    password VARCHAR(50) NOT NULL
);

-- 3. 插入一个测试用户 (账号: admin, 密码: 123)
INSERT INTO users (username, password) VALUES ('admin', '123');

-- 4. 验证一下
SELECT * FROM users;

-- ==========================================================
--5. 创建设备表
-- 一个用户(user_id) 可以拥有多个设备。
-- 每个设备有一个唯一的标识符 (device_uuid)，比如 MAC 地址或我们生成的 ID。
-- status 用来标记设备是否在线 (0:离线, 1:在线)。
-- ==========================================================
CREATE TABLE IF NOT EXISTS devices(
    id INT AUTO_INCREMENT PRIMARY KEY,  -- 关联到user表的主表
    user_id INT NOT NULL,                 -- 关联到 users 表的主键
    device_name VARCHAR(100) NOT NULL,  -- 设备名字
    device_uuid VARCHAR(64) NOT NULL UNIQUE, -- 设备id码，唯一标识
    device_ip    VARCHAR(64), -- 设备当前的ip地址
    rtsp_url    VARCHAR(255), -- 摄像头的rtsp 流地址
    status TINYINT DEFAULT 0,  -- 0是不在线， 1是设备在线
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,  -- 创建时间, 如果没有赋值则是用当前的时间值
    FOREIGN KEY (user_id) REFERENCES user_id(id) ON DELETE CASCADE
);
-- 插入一条测试数据
INSERT INTO devices (user_id, device_name, device_uuid, rtsp_url, status)
VALUES (1, 'NVIDIA 2080Ti Edge Node', 'uuid-edge-001', 'rtsp://39.104.79.45/stream1', 1);
-- 验证一下
SELECT * FROM devices;

