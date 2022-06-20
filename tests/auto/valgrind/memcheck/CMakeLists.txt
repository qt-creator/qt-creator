add_subdirectory(testapps)

include(../valgrind.cmake)

file(RELATIVE_PATH RELATIVE_TEST_PATH "${PROJECT_BINARY_DIR}" "${CMAKE_CURRENT_BINARY_DIR}")
file(RELATIVE_PATH TEST_RELATIVE_LIBEXEC_PATH "/${RELATIVE_TEST_PATH}" "/${IDE_LIBEXEC_PATH}")

add_qtc_test(modeldemo
  MANUALTEST
  DEPENDS
    Utils
    Core
    Debugger
    ProjectExplorer
    TextEditor

  SOURCES
    modeldemo.cpp
    modeldemo.h

  DEFINES
    PARSERTESTS_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
    VALGRIND_FAKE_PATH="${PROJECT_BINARY_DIR}/src/tools/valgrindfake/valgrind-fake"
    TEST_RELATIVE_LIBEXEC_PATH="${TEST_RELATIVE_LIBEXEC_PATH}"
)

extend_valgrind_test(modeldemo)
