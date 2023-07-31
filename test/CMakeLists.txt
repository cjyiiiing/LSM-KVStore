# 创建一个名为correctness的可执行文件，其源文件为correctness.cc
add_executable(test_kvstore test_kvstore.cc)
# 将可执行文件correctness与名为lsmstore的库进行链接
target_link_libraries(test_kvstore lsmstore)

add_executable(test_cache test_cache.cc)
target_link_libraries(test_cache lsmstore)

# add_executable(test_alloc test_alloc.cc)
# target_link_libraries(test_alloc lsmstore)