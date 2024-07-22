#ifndef FRIENDMODEL_H
#define FRIENDMODEL_H

#include "user.hpp"
#include "connectionpool.hpp"
#include <vector>
using namespace std;

//friend表操作接口方法
class FriendModel
{
public:
    FriendModel();
    //添加好友关系
    void insert(int userid, int friendid);

    //返回用户好友列表
    vector<User> query(int userid);

    shared_ptr<MySQL> _sp;

};


#endif