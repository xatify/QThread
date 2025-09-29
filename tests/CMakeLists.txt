add_executable(test_threadpool test_threadpool.cpp)
target_link_libraries(test_threadpool PRIVATE qthread)
add_test(NAME ThreadPoolTest COMMAND test_threadpool)

