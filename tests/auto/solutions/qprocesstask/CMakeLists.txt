add_subdirectory(testsleep)

add_qtc_test(tst_qprocesstask
  DEFINES "PROCESS_TESTAPP=\"${CMAKE_CURRENT_BINARY_DIR}/testsleep/testsleep\""
  DEPENDS Qt::Network Qt::TaskTree
  SOURCES tst_qprocesstask.cpp
)
