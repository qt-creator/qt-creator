add_subdirectory(testsleep)

add_qtc_test(tst_qprocesstask
  DEFINES "PROCESS_TESTAPP=\"${CMAKE_CURRENT_BINARY_DIR}/testsleep\""
  DEPENDS Tasking Qt::Network
  SOURCES tst_qprocesstask.cpp
)
