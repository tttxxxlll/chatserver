#include "json.hpp"
using json = nlohmann::json;

#include <iostream>
#include <vector>
#include <map>
#include <string>
using namespace std;

string func1() {
    json js;
    js["msg_type"] = 2;
    js["from"] = "zhang san";
    js["to"] = "li si";
    js["msg"] = "hello, what are you doing?";

    string sendBuf = js.dump(); // 将序列化之后的js保存到字符串中
    //cout << sendBuf.c_str() << endl; // .c_str()表示将sendbuf字符串转换为网络传输需要的char*类型；
    //cout << js << endl;

    return sendBuf;
}

void func2() {
    json js;
    // 添加数组
    js["id"] = {1,2,3,4,5}; 
    // 添加key-value
    js["name"] = "zhang san"; 
    // 添加对象
    js["msg"]["zhang san"] = "hello world";
    js["msg"]["liu shuo"] = "hello china"; 
    // 上面等同于下面这句一次性添加数组对象
    js["msg"] = {{"zhang san", "hello world"}, {"liu shuo", "hello china"}};
    cout << js << endl;
}

void func3() {
    json js;
    // 直接序列化一个vector容器
    vector<int> vec;
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(5);
    js["list"] = vec;
    // 直接序列化一个map容器
    map<int, string> m;
    m.insert({1, "黄山"});
    m.insert({2, "华山"});
    m.insert({3, "泰山"});
    js["path"] = m;
    cout<<js<<endl;
}

int main() {
    string recvBuf = func1();
    json jsbuf = json::parse(recvBuf); //将收到的字符串反序列化
    // cout << jsbuf["msg_type"] << endl;
    // cout << jsbuf["from"] << endl;
    // cout << jsbuf["to"] << endl;
    // cout << jsbuf["msg"] << endl;
    cout << jsbuf <<endl;

    return 0;
}

