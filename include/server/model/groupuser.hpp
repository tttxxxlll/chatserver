#ifndef GROUPUSER_H
#define FROUPUSER_H
#include "user.hpp"

//群组用户不仅要有user表中的信息，还要多一个群内role信息
class GroupUser : public User
{
public:
    void setRole(string role) {this->role = role;}
    string getRole() {return this->role;}
private:
    string role; 
};



#endif