#ifndef CHATSERVICE_H
#define CHAYSERVICE_H

#include <unordered_map>
#include <functional>
#include <mutex>
#include "json.hpp"
#include <muduo/net/TcpConnection.h>
#include "usermodel.hpp"
#include "offlinemessagemodel.hpp"
#include "friendmodel.hpp"
#include "groupmodel.hpp"
#include "redis.hpp"
using namespace std;
using json = nlohmann::json;
using namespace muduo;
using namespace muduo::net;

//表示处理消息的事件回调方法类型
using MsgHandler = function<void(const TcpConnectionPtr &conn, json &js, Timestamp time)>;

//聊天服务器的业务类（单例模式）
class ChatService
{
public:
    //获取单例对象的接口函数
    static ChatService* instance();
    //处理登录业务
    void login(const TcpConnectionPtr &conn, json &js, Timestamp time);
    //处理注册业务
    void reg(const TcpConnectionPtr &conn, json &js, Timestamp time);
    //一对一聊天业务
    void oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time);
    //添加好友业务
    void addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time);
    //创建群组业务
    void createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    //加入群组业务
    void addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    //群组聊天业务
    void groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time);
    //获取消息对应的处理器
    MsgHandler getHandler(int msgid);
    //处理client异常退出
    void clientCloseException(const TcpConnectionPtr &conn);
    //处理注销登录业务
    void loginout(const TcpConnectionPtr &conn, json &js, Timestamp time);
    //服务器异常重置
    void reset();
    //redis上报消息业务
    void handlerRsdisSubscribeMessage(int channel, string message);

    
private:
    ChatService();

    unordered_map<int, MsgHandler> _msgHandlerMap;

    //数据操作对象类
    UserModel _userModel;
    OfflineMsgModel _offlineMsgModel;
    FriendModel _friendModel;
    GroupModel _groupModel;

    //之后要聊天，所以要存储在线用户的连接信息ip and port
    unordered_map<int, TcpConnectionPtr> _userConnMap;

    //定义互斥锁，保证_userconnmap的线程安全
    mutex _connMutex;

    //服务器通信的消息队列对象
    Redis _redis;

};



#endif