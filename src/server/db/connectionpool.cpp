#include "connectionpool.hpp"
#include <muduo/base/Logging.h>
#include <iostream>
using namespace std;

ConnectionPool *ConnectionPool::getConnectionpool()
{
    static ConnectionPool pool;
    return &pool;
}

ConnectionPool::ConnectionPool()
{
    // if (!loadConfigFile())
    // {
    //     exit(0);
    // }
    _ip = "127.0.0.1";
    _port = 3306;
    _username = "root";
    _password = "123456";
    _dbname = "chat";
    _initSize = 10;
    _maxSize = 1024;
    _maxIdletime = 60;
    _ConnectionTimeout = 1000;

    for (int i = 0; i < _initSize; i++)
    {
        MySQL *p = new MySQL();
        if (p->connect(_ip, _port, _username, _password, _dbname))
        {
            p->refreshalivetime();
            _connectionQue.push(p);
            _connectionCnt++;
        }
        else
        {
            continue;
        }
    }

    // 创建生产者线程
    thread produce(bind(&ConnectionPool::produceConnectionTask, this));
    produce.detach();
    // 创建一个回收超过maxidletime空闲connection的线程
    thread scanner(bind(&ConnectionPool::scannerConnectionTask, this));
    scanner.detach();
}

shared_ptr<MySQL> ConnectionPool::getConnection()
{
    unique_lock<mutex> lock(_queMutex);
    while (_connectionQue.empty())
    {
        // 如果是超时唤醒再判断是否为空
        if (cv_status::timeout == cv.wait_for(lock, chrono::milliseconds(_ConnectionTimeout)))
        {
            if (_connectionQue.empty())
            {
                LOG_INFO << "get connection timeout!";
            }
        }
    }
    // 智能指针析构时会delete掉connection，需要自定义智能指针的释放资源的方式，把connection归还到queue中
    shared_ptr<MySQL> sp(_connectionQue.front(),
                         [&](MySQL *pcon)
                         {
                             unique_lock<mutex> lock(_queMutex);
                             pcon->refreshalivetime();
                             _connectionQue.push(pcon);
                         });
    _connectionQue.pop();

    cv.notify_all(); // 消费完之后广播，唤醒别的生产者或者消费者

    return sp;
}

void ConnectionPool::produceConnectionTask()
{
    for (;;)
    {
        unique_lock<mutex> lock(_queMutex);
        while (!_connectionQue.empty())
        {
            cv.wait(lock); // 队列不为空，阻塞在条件变量上，并且会释放锁
        }

        if (_connectionCnt < _maxSize)
        {
            MySQL *p = new MySQL();
            if (p->connect(_ip, _port, _username, _password, _dbname))
            {
                p->refreshalivetime();
                _connectionQue.push(p);
                _connectionCnt++;
            }
        }
        cv.notify_all();
    }
}

void ConnectionPool::scannerConnectionTask()
{
    for (;;)
    {
        this_thread::sleep_for(chrono::seconds(_maxIdletime));
        unique_lock<mutex> lock(_queMutex);
        while (_connectionCnt > _initSize)
        {
            MySQL *p = _connectionQue.front();
            if (p->getalivetime() >= _maxIdletime * 1000)
            {
                _connectionQue.pop();
                _connectionCnt--;
                delete p;
            }
            else
            {
                break; // 队头没有超过，其他connection肯定没有
            }
        }
    }
}