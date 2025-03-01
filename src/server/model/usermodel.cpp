#include "usermodel.hpp"
#include "db.h"
#include <iostream>
using namespace std;

UserModel::UserModel() {
    ConnectionPool *p = ConnectionPool::getConnectionpool();
    _sp = p->getConnection();
}

bool UserModel::insert(User &user)
{

    // 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into User(name, password, state) values('%s', '%s', '%s')",
            user.getName().c_str(), user.getPwd().c_str(), user.getState().c_str()); // c_str()的作用是将string转成char*

    if (_sp->update(sql))
    {
        // 获取插入成功的用户数据的主键id
        user.setId(mysql_insert_id(_sp->getConnection()));
        return true;
    }

    return false;
}

// 根据用户号码查询用户信息
User UserModel::query(int id)
{
    // 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select * from User where id = %d", id);

    MYSQL_RES *res = _sp->query(sql);
    if (res != nullptr)
    {
        MYSQL_ROW row = mysql_fetch_row(res); // 获取行数据
        if (row != nullptr)
        {
            User user;
            user.setId(atoi(row[0]));
            user.setName(row[1]);
            user.setPwd(row[2]);
            user.setState(row[3]);
            mysql_free_result(res); // res在堆区，要手动释放
            return user;
        }
    }

    return User();
}

// 更新用户的状态信息
bool UserModel::updatestate(User user)
{
    // 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "update User set state = '%s' where id = %d", user.getState().c_str(), user.getId()); // c_str()的作用是将string转成char*

    if (_sp->update(sql))
    {
        return true;
    }

    return false;
}

//重置用户的状态信息
void UserModel::resetState() {
    char sql[1024] = "update User set state = 'offline' where state = 'online'";
    _sp->update(sql);

}