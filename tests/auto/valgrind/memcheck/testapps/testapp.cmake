function(add_valgrind_testapp name)
  add_executable("${name}"
                 ${CMAKE_CURRENT_LIST_DIR}/main.cpp)
  set_target_properties(${name} PROPERTIES QT_COMPILE_OPTIONS_DISABLE_WARNINGS ON)

endfunction()
