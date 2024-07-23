# chatserver
A clustered chatserver based on the muduo library working in a nginx tcp load-balanced environment, using redis as the message queue to complete the server communication and mysql as the database to save user data.

Quick start:
  在nginx负载均衡器中配置各服务器的ip + port.
  在项目根目录下执行./autobuild.sh
  可执行文件会放入bin文件夹中。
  1. ./ChatServer ip port 以启动服务器
  2. ./ChatClient ip 8000 以启动客户端

需要的环境以及三方库：Json  boost+muduo  nginx redis  mysql  CMake
  
