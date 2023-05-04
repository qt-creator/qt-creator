add_subdirectory(processtestapp)

file(RELATIVE_PATH RELATIVE_TEST_PATH "${PROJECT_BINARY_DIR}" "${CMAKE_CURRENT_BINARY_DIR}")
file(RELATIVE_PATH TEST_RELATIVE_LIBEXEC_PATH "/${RELATIVE_TEST_PATH}" "/${IDE_LIBEXEC_PATH}")

add_qtc_test(tst_process
  DEFINES "TEST_RELATIVE_LIBEXEC_PATH=\"${TEST_RELATIVE_LIBEXEC_PATH}\""
          "PROCESS_TESTAPP=\"${CMAKE_CURRENT_BINARY_DIR}/processtestapp\""
  DEPENDS Utils app_version
  SOURCES tst_process.cpp
          processtestapp/processtestapp.h
          processtestapp/processtestapp.cpp
)
