include(../testapp.cmake)

add_valgrind_testapp(leak4)
target_link_libraries(leak4 PRIVATE Qt::Core)
