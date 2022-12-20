include(../valgrind.cmake)

add_qtc_test(tst_callgrindparsertests
  DEPENDS Utils Core ProjectExplorer Debugger Qt::Core Qt::Network
  DEFINES
    PARSERTESTS_DATA_DIR="${CMAKE_CURRENT_LIST_DIR}/data"
    CALLGRINDPARSERTESTS
  INCLUDES "${PROJECT_SOURCE_DIR}/src/plugins"
  SOURCES callgrindparsertests.cpp callgrindparsertests.h
)

extend_valgrind_test(tst_callgrindparsertests)

# skipping modeltest (does not compile due to missing widget handler)
