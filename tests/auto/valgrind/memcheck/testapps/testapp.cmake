function(add_valgrind_testapp name)
  add_executable("${name}"
                 ${CMAKE_CURRENT_LIST_DIR}/main.cpp)
endfunction()
