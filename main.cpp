#include "./include/webserver.hpp"

/*
响应模式
    0：连接和监听都是LT
    1：连接ET，监听LT
    2：连接LT，监听ET
    3：连接和监听都是ET
*/
int main() {
    WebServer server(
        1316, 3, 60000, // client监听端口, ET触发模式, 连接定时1min
        false, 4    // 关闭延时退出, 线程池中的线程数
    );
    server.Start();
}