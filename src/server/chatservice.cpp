#include "chatservice.hpp"
#include "public.hpp"
#include <muduo/base/Logging.h>
#include <vector>
using namespace std;
using namespace muduo;

// 获取单例对象的接口函数
ChatService *ChatService::instance()
{
    static ChatService service;
    return &service;
}

ChatService::ChatService()
{
    _msgHandlerMap.insert({LOGIN_MSG, bind(&ChatService::login, this, _1, _2, _3)});
    _msgHandlerMap.insert({REG_MSG, bind(&ChatService::reg, this, _1, _2, _3)});
    _msgHandlerMap.insert({ONE_CHAT_MSG, bind(&ChatService::oneChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_FRIEND_MSG, bind(&ChatService::addFriend, this, _1, _2, _3)});
    _msgHandlerMap.insert({CREATE_GROUP_MSG, bind(&ChatService::createGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_GROUP_MSG, bind(&ChatService::addGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG, bind(&ChatService::groupChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({LOGINOUT_MSG, bind(&ChatService::loginout, this, _1, _2, _3)});

    if (_redis.connect())
    {
        // 设置上报消息的回调
        _redis.init_notify_handler(std::bind(&ChatService::handlerRsdisSubscribeMessage, this, _1, _2));
    }
}

// 获取消息对应的处理器
MsgHandler ChatService::getHandler(int msgid)
{
    if (auto it = _msgHandlerMap.find(msgid) == _msgHandlerMap.end())
    {
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp time)
        {
            LOG_ERROR << "msgid: " << msgid << "can not find handler!";
        };
    }
    else
        return _msgHandlerMap[msgid];
}

// 处理登录业务
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int id = js["id"];
    string pwd = js["password"];

    User user = _userModel.query(id);
    if (user.getId() == id && user.getPwd() == pwd)
    {
        if (user.getState() == "online")
        {
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = "this client is already login!";
            conn->send(response.dump());
        }
        else
        {
            // 登陆成功，记录用户连接信息
            { // 这个大括号是为了让锁只在大括号内作用，因为后续的操作不需要锁，因此尽量保证锁的粒度小
                // 大括号结束时就会析构解锁
                lock_guard<mutex> lock(_connMutex); // 使用这个类定义对象并传入锁，这个对象就可以通过构造和析构自行加锁解锁
                _userConnMap.insert({id, conn});    // 多线程同时插入时会出现线程安全问题，所以要加锁
            }

            // id用户登录成功，向redis订阅channel（id），来接收登录在其他服务器上的用户发给id的消息
            _redis.subscribe(id);

            // 登录成功，更新用户状态信息
            user.setState("online");
            _userModel.updatestate(user);
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["id"] = user.getId();
            response["name"] = user.getName();
            // 查询是否有离线消息
            vector<string> vec = _offlineMsgModel.query(id);
            if (!vec.empty())
            {
                response["offlinemsg"] = vec;
                // 读取完成之后从数据库删除离线消息
                _offlineMsgModel.remove(id);
            }

            // 查询用户的好友信息并返回
            vector<User> uservec = _friendModel.query(id);
            if (!uservec.empty())
            {
                vector<string> vec2;
                for (User &user : uservec)
                {
                    json js;
                    js["id"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = user.getState();
                    vec2.push_back(js.dump());
                }
                response["friends"] = vec2;
            }

            // 查询用户群组信息并返回
            vector<Group> grpvec = _groupModel.queryGroups(id);
            if (!grpvec.empty())
            {
                vector<string> vec3;
                for (Group &group : grpvec)
                {
                    json js;
                    js["id"] = group.getId();
                    js["groupname"] = group.getName();
                    js["groupdesc"] = group.getDesc();

                    vector<string> vec4;
                    for (GroupUser &user : group.getUsers())
                    {
                        json userjs;
                        userjs["id"] = user.getId();
                        userjs["name"] = user.getName();
                        userjs["state"] = user.getState();
                        userjs["role"] = user.getRole();
                        vec4.push_back(userjs.dump());
                    }
                    js["users"] = vec4;
                    vec3.push_back(js.dump());
                }
                response["groups"] = vec3;
            }

            conn->send(response.dump());
        }
    }
    else
    {
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "name or password is invalid!";
        conn->send(response.dump());
    }
}
// 处理注册业务 name password
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    string name = js["name"];
    string pwd = js["password"];
    User user;
    user.setName(name);
    user.setPwd(pwd);
    if (_userModel.insert(user))
    {
        // 注册成功
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 0;
        response["id"] = user.getId();
        response["name"] = user.getName();
        conn->send(response.dump());
    }
    else
    {

        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        conn->send(response.dump());
    }
}

// 处理client异常退出
void ChatService::clientCloseException(const TcpConnectionPtr &conn)
{
    User user;
    { // 从connmap中删除连接信息
        lock_guard<mutex> lock(_connMutex);
        for (auto it = _userConnMap.begin(); it != _userConnMap.end(); it++)
        {
            if (it->second == conn)
            {
                user.setId(it->first);
                _userConnMap.erase(it);
                break;
            }
        }
    }

    _redis.unsubscribe(user.getId());

    // 更新数据库中的状态信息
    if (user.getId() != -1)
    {
        user.setState("offline");
        _userModel.updatestate(user);
    }
}

// 处理注销登录业务
void ChatService::loginout(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    User user;
    int userid = js["id"].get<int>();
    user.setId(userid);

    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(userid);
        if (it != _userConnMap.end())
        {
            _userConnMap.erase(it);
        }
    }

    // 用户下线，在redis中取消订阅通道
    _redis.unsubscribe(userid);

    user.setState("offline");
    _userModel.updatestate(user);
}

// 服务器异常重置
void ChatService::reset()
{
    _userModel.resetState();
}

// 一对一聊天业务
void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int toid = js["toid"].get<int>();
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(toid);
        if (it != _userConnMap.end())
        {
            // toid在线，转发消息，服务器推送消息给toid
            it->second->send(js.dump());
            return;
        }
    }

    // 查询toid是否在线
    User user = _userModel.query(toid);
    if (user.getState() == "online")
    {
        _redis.publish(toid, js.dump());
        return;
    }

    // toid不在线，存储离线消息
    _offlineMsgModel.insert(toid, js.dump());
}

// 添加好友业务
void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    _friendModel.insert(userid, friendid);
}

// 创建群组业务
void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    string name = js["groupname"];
    string desc = js["groupdesc"];

    Group group(-1, name, desc);
    if (_groupModel.createGroup(group))
    {
        // 将创建人加入群组
        _groupModel.addGroup(userid, group.getId(), "creator");
    }
}

// 加入群组业务
void ChatService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    _groupModel.addGroup(userid, groupid, "normal");
}

// 群组聊天业务
void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();

    vector<int> useridVec = _groupModel.queryGroupUsers(userid, groupid);
    lock_guard<mutex> lock(_connMutex);
    for (int id : useridVec)
    {
        auto it = _userConnMap.find(id);
        if (it != _userConnMap.end())
        {
            it->second->send(js.dump());
        }
        else
        {
            User user = _userModel.query(id);
            if (user.getState() == "online")
            {
                _redis.publish(id, js.dump());
            }
            else
            {
                _offlineMsgModel.insert(id, js.dump());
            }
        }
    }
}

// redis上报消息业务
void ChatService::handlerRsdisSubscribeMessage(int userid, string message)
{
    lock_guard<mutex> lock(_connMutex);
    auto it = _userConnMap.find(userid);
    if (it != _userConnMap.end())
    {
        // toid在线，转发消息，服务器推送消息给toid
        it->second->send(message);
        return;
    }
    // toid不在线，存储离线消息
    _offlineMsgModel.insert(userid, message);
}