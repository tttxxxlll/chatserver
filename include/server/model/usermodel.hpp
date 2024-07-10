#ifndef USERMODEL_H
#define USERMODEL_H
#include "user.hpp"
//user表的数据操作类
class UserModel {
public:
    bool insert(User &user);

    //根据用户号码查询用户信息
    User query(int id);

    //更新用户的状态信息
    bool updatestate(User user);

    //重置用户的状态信息
    void resetState();
};

#endif
