# TCP-Lab
基于C++实现的用户态TCP软件，实现了以下TCP特性
1. 字符乱序重组（增量更新）
2. TCP序列号系统
3. TCP接收方（接收TCP报文，回送ACK和流量控制）
4. TCP发送方（累计确认，定时器和超时重传机制）
5. 全双工的TCP连接（三次握手四次挥手）
## 使用方法
```shell script
mkdir build && cd build
cmake ..
make
```