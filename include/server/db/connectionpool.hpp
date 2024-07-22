#ifndef CONNECTIONPOOL_H
#define CONNECTIONPOOL_H
#include <string>
#include <queue>
#include <mutex>
#include <atomic>
#include <thread>
#include <memory>
#include <functional>
#include <condition_variable>
#include "db.h"
using namespace std;

class ConnectionPool {
public:
    static ConnectionPool *getConnectionpool();
    //给外部的接口，从连接池中获取一个可用的连接
    shared_ptr<MySQL> getConnection();//用智能指针接收可以不用用户手动归还
private:
    ConnectionPool();

    void produceConnectionTask();//生产者线程的回调函数

    void scannerConnectionTask();//回收超过maxidletime空闲connection的线程的回调函数

    string _ip;
    unsigned short _port;
    string _username;
    string _password;
    string _dbname;
    int _initSize;
    int _maxSize;
    int _maxIdletime;
    int _ConnectionTimeout;
    queue<MySQL*> _connectionQue; //存放连接
    mutex _queMutex;
    atomic_int _connectionCnt;//记录所创建的连接的总数量，为了保证线程安全，所以使用atomic原子类型
    condition_variable cv; //设置条件变量， 用于连接生产线程和连接消费线程的的通信

};



#endif