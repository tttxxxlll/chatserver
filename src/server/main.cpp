#include "chatserver.hpp"
#include "chatservice.hpp"
#include <signal.h>
#include <iostream>
using namespace std;

void resethandler(int)
{
    ChatService::instance()->reset();
    exit(0);
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        cerr << "command invalid! example: ./ChatServer 127.0.0.1 6000" << endl;
        exit(-1);
    }

    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);

    signal(SIGINT, resethandler);//用信号捕捉函数处理ctrl c服务器异常退出
    EventLoop loop;
    InetAddress addr(ip, port);
    ChatServer server(&loop, addr, "ChatServer");

    server.start();
    loop.loop();

    return 0;
}