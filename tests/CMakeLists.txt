add_executable(test_threadpool test_threadpool.cpp)
target_link_libraries(test_threadpool PRIVATE qthread)
add_test(NAME ThreadPoolTest COMMAND test_threadpool)

add_executable(test_prime test_prime.cpp)
target_link_libraries(test_prime PRIVATE qthread)
add_test(NAME PrimeTest COMMAND test_prime)


add_executable(test_tree test_tree.cpp)
target_link_libraries(test_tree PRIVATE qthread)
add_test(NAME TreeTest COMMAND test_tree)
