//两个类TcpServer\TcpClient
#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <iostream>
#include <string>
#include <functional>
using namespace std;
using namespace muduo;
using namespace muduo::net;
using namespace placeholders;

//基于muduo网络库开发服务器程序
//1. 组合tcpserver对象
//2. 创建eventloop事件循环对象指针
//3. 明确tcpserver构造函数需要什么参数， 输出chatserver的构造函数
//4. 在当前chatserver类的构造函数中注册处理连接和处理读写事件的回调函数
//5. 设置合适的服务端线程数量

class ChatServer
 {
public:
    ChatServer(EventLoop* loop,         //事件循环reactor
            const InetAddress& listenAddr,//IP+port
            const string& nameArg)        //服务器的名字
        :_server(loop, listenAddr, nameArg), _loop(loop)
    {
        //给服务器注册用户连接的创建和断开设置回调
        _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, placeholders::_1));//当事件满足时自动调用，会用包装器将绑定器得到的仿函数包装成函数，默认传参
        
        //给服务器注册用户读写事件设置回调
        _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));

        //设置服务器端的线程数量
        _server.setThreadNum(4);//相当于一个线程做io线程（处理新用户连接），另3个做工作线程（处理读写事件）
    }

    //开启事件循环
    void start() {
        _server.start();
    }

private:
    //专门处理用户的连接创建和断开   
    void onConnection(const TcpConnectionPtr &conn) { // 参数是一个智能指针
        if (conn->connected()) {
            cout << conn->peerAddress().toIpPort() << " -> " << conn->localAddress().toIpPort() << " state:online " << endl;
        } else {
            cout << conn->peerAddress().toIpPort() << " -> " << conn->localAddress().toIpPort() << " state:offline " << endl;
            conn->shutdown(); //close(fd);
            //_loop->quit();
        }
    }


    //专门处理用户的读写事件 
    void onMessage(const TcpConnectionPtr &conn,//连接
                    Buffer *buffer,//读写缓冲区
                    Timestamp time) //接收导数据的时间信息
    {
        string buf = buffer->retrieveAllAsString();
        cout << "recv data:" << buf << " time:" << time.toString() << endl;
        conn->send(buf);
    }

    TcpServer _server;
    EventLoop *_loop;

 };

 int main() {

    EventLoop loop; //epoll
    InetAddress addr("127.0.0.1", 6000);
    ChatServer server(&loop, addr, "ChatServer");

    server.start(); //lfd上epoll树
    loop.loop();// epoll_wait 阻塞等待新用户连接或已连接用户的读写事件等

    return 0;
 }
