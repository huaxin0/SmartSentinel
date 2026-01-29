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