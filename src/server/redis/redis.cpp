#include "redis.hpp"
#include <iostream>
using namespace std;

Redis::Redis() : _publish_context(nullptr), _subcribe_context(nullptr)
{
}
Redis::~Redis()
{
    if (_publish_context != nullptr)
    {
        redisFree(_publish_context);
    }

    if (_subcribe_context != nullptr)
    {
        redisFree(_subcribe_context);
    }
}

// 连接redis服务器
bool Redis::connect()
{
    _publish_context = redisConnect("127.0.0.1", 6379);
    if (_publish_context == nullptr)
    {
        cerr << "connect redis error!" << endl;
    }
    _subcribe_context = redisConnect("127.0.0.1", 6379);
    if (_publish_context == nullptr)
    {
        cerr << "connect redis error!" << endl;
    }

    // 再单独的线程中监听通道上的事件，有消息给业务层进行上报，单独线程主要是因为订阅之后会阻塞
    thread t([&]()
             { observer_channel_message(); });
    t.detach();
    cout << "connect redis-server success!" << endl;
    return true;
}

// 向redis指定的通道channel发布消息 //publish命令不会阻塞
bool Redis::publish(int channel, string message)
{
    redisReply *reply = (redisReply *)redisCommand(_publish_context, "PUBLISH %d %s", channel, message.c_str());
    if (reply == nullptr)
    {
        cerr << "publish command error!" << endl;
        return false;
    }
    freeReplyObject(reply);
    return true;
}

// 向redis指定的通道subscribe订阅消息
bool Redis::subscribe(int channel)
{
    // subscribe命令本身会阻塞线程等待通道里面产生消息，这里只做订阅通道，不接受通道消息
    // 通道消息的接收专门在observer_channel_message函数中的独立线程中进行
    // 只负责发送命令，不阻塞等待响应消息，否则会和notifymsg线程抢占资源
    if (REDIS_ERR == redisAppendCommand(this->_subcribe_context, "SUBSCRIBE %d", channel))
    {
        cerr << "subscribe commend error!" << endl;
        return false;
    }
    // redisbufferwrite可以循环发送缓冲区，直到缓冲区数据发送完毕
    int done = 0;
    while (!done)
    {
        if (REDIS_ERR == redisBufferWrite(this->_subcribe_context, &done))
        {
            cerr << "subscribe commend error!" << endl;
            return false;
        }
    }
    return true;
}

////向redis指定的通道unsubscribe取消订阅消息
bool Redis::unsubscribe(int channel)
{
    if (REDIS_ERR == redisAppendCommand(this->_subcribe_context, "UNSUBSCRIBE %d", channel))
    {
        cerr << "unsubscribe commend error!" << endl;
        return false;
    }
    // redisbufferwrite可以循环发送缓冲区，直到缓冲区数据发送完毕
    int done = 0;
    while (!done)
    {
        if (REDIS_ERR == redisBufferWrite(this->_subcribe_context, &done))
        {
            cerr << "unsubscribe commend error!" << endl;
            return false;
        }
    }
    return true;
}

// 在独立线程中接收订阅通道中的消息
void Redis::observer_channel_message()
{
    redisReply *reply = nullptr;
    while (REDIS_OK == redisGetReply(this->_subcribe_context, (void **)&reply))
    {
        // 订阅收到的消息是一个带三元素的数组
        if (reply != nullptr && reply->element[2] != nullptr && reply->element[2]->str != nullptr)
        {
            // 给业务层上报通道上发生的消息
            _notify_message_handler(atoi(reply->element[1]->str), reply->element[2]->str);
        }

        freeReplyObject(reply);
    }
}

// 初始化向业务层上报通道消息的回调对象
void Redis::init_notify_handler(function<void(int, string)> fn)
{
    _notify_message_handler = fn;
}