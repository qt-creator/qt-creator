include(../testapp.cmake)

add_valgrind_testapp(leak1)
target_link_libraries(leak1 PRIVATE Qt::Core)

