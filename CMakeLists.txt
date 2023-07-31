# 指定要求的CMake最低版本
cmake_minimum_required(VERSION 3.10.2)
# 设置项目名称
project(LSMKV)

# 设置C++标准为C++17
set(CMAKE_CXX_STANDARD 17)
# 设置编译选项，此处为启用调试信息
set(CMAKE_CXX_FLAGS "-g -pthread")

# 设置可执行文件输出路径和库文件输出路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)

# 包含相关目录，以寻找头文件
include_directories(${PROJECT_SOURCE_DIR}/include)
# include_directories(${PROJECT_SOURCE_DIR}/include/cache)

# 包含相关目录，以寻找链接库文件
link_directories(${PROJECT_SOURCE_DIR}/lib)

# 添加子目录
add_subdirectory(src)
add_subdirectory(test)