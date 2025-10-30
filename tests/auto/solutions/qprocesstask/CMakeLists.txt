add_subdirectory(testsleep)

if(Qt6_VERSION VERSION_LESS 6.11.0)
  set(TaskTree QtTaskTree)
else()
  set(TaskTree Qt::TaskTree)
endif()

add_qtc_test(tst_qprocesstask
  DEFINES "PROCESS_TESTAPP=\"${CMAKE_CURRENT_BINARY_DIR}/testsleep/testsleep\""
  DEPENDS Qt::Network TaskTree
  SOURCES tst_qprocesstask.cpp
)
