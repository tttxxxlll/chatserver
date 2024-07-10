#include "chatserver.hpp"
#include "json.hpp"
#include "chatservice.hpp"
#include <functional>
#include <string>
using namespace std;
using namespace placeholders;
using json = nlohmann::json;

// 初始化服务器对象
ChatServer::ChatServer(EventLoop *loop,
                       const InetAddress &listenAddr,
                       const string &nameArg)
    : _server(loop, listenAddr, nameArg), _loop(loop)
{
    // 注册连接回调
    _server.setConnectionCallback(bind(&ChatServer::onConnection, this, _1));
    // 注册读写事件回调
    _server.setMessageCallback(bind(&ChatServer::onMessage, this, _1, _2, _3));
    // 设置线程数量
    _server.setThreadNum(4);
}

// 启动服务
void ChatServer::start()
{
    _server.start();
}

// 连接信息的回调函数
void ChatServer::onConnection(const TcpConnectionPtr &conn)
{
    // client断开连接
    if (!conn->connected())
    {
        //client异常退出处理
        ChatService::instance()->clientCloseException(conn);
        conn->shutdown();
    }
}

// 处理读写事件的回调函数
void ChatServer::onMessage(const TcpConnectionPtr &conn,
                           Buffer *buffer,
                           Timestamp time)
{
    string buf = buffer->retrieveAllAsString();
    // 数据的反序列化
    json js = json::parse(buf);
    // 通过回调解耦网络模块的代码和业务模块的代码
    // 通过js["msgid"]获取业务的处理方式，还可传参：conn， js， time
    auto msgHandler = ChatService::instance()->getHandler(js["msgid"].get<int>()); /*get是将js中的数据转换成真正的int，
     gethandler函数返回的就是map中的value，也就是绑定器绑定的回调函数*/
    msgHandler(conn, js, time);
    //ChatService::instance()->_msgHandlerMap[js["msgid"].get<int>()](conn, js, time);
}
