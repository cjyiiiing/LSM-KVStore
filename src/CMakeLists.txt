# 将当前目录下的所有源代码文件添加到变量SRC_LIST中
aux_source_directory(. SRC_LIST)

# 创建一个名为lsmstore的共享库，并使用变量SRC_LIST中的源代码文件进行构建。SHARED表示创建一个共享库（或动态链接库）
add_library(lsmstore SHARED ${SRC_LIST})