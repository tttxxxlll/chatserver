#配置头文件搜索路径
#include_directories()
#配置库文件搜索路径
#link_directories()

#设置需要编译的源文件列表
set(SRC_LIST ./muduo_server.cpp)

#设置可执行文件最终存储的路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

#把指定路径的所有源文件名字放入变量中
#aux_source_directory(路径名 SRC_LIST)

#用src_list生成可执行文件server
add_executable(server ${SRC_LIST})

#添加可执行文件需要连接的库名
target_link_libraries(server muduo_net muduo_base pthread)