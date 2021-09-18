# 数据包格式

每段消息由数据头(1)和数据本身(>=1)至少两个包组成

## 数据头

[unsigned]

* unsigned
  * 0 断开连接
  * 1 获取时间
  * 2 获取名字
  * 3 活动连接列表
  * 4 发消息
  * 5 消息报错

## 数据

### 获取时间

[std::time_t]

* std::time_t
  * 时间

### 获取名字

[size_t, [char]*size_t]

* size_t
  * 字符串长度（不含 `\0`）
* [char]*size_t
  * 名字

### 活动连接列表

[size_t, [int, struct sockaddr]*size_t]

* size_t
  * 活动连接个数
* [int, struct sockaddr]*size_t
  * int: 编号
  * struct sockaddr: socket 信息

### 发消息

[int, size_t, char*size_t]

* int
  * 编号
* size_t
  * 字符串长度（不含 `\0`）
* [char]*size_t
  * 消息
