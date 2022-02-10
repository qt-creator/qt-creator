file(RELATIVE_PATH RELATIVE_TEST_PATH "${PROJECT_BINARY_DIR}" "${CMAKE_CURRENT_BINARY_DIR}")
file(RELATIVE_PATH TEST_RELATIVE_LIBEXEC_PATH "/${RELATIVE_TEST_PATH}" "/${IDE_LIBEXEC_PATH}")

add_qtc_test(tst_manual_shell
  MANUALTEST
  CONDITION UNIX
  DEPENDS Utils QtcSsh Qt5::Network
  DEFINES "TEST_RELATIVE_LIBEXEC_PATH=\"${TEST_RELATIVE_LIBEXEC_PATH}\""
  SOURCES
    argumentscollector.cpp argumentscollector.h
    main.cpp
    shell.cpp shell.h
)
