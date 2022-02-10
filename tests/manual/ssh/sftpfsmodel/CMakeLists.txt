file(RELATIVE_PATH RELATIVE_TEST_PATH "${PROJECT_BINARY_DIR}" "${CMAKE_CURRENT_BINARY_DIR}")
file(RELATIVE_PATH TEST_RELATIVE_LIBEXEC_PATH "/${RELATIVE_TEST_PATH}" "/${IDE_LIBEXEC_PATH}")

add_qtc_test(tst_manual_sftpfsmodel
  MANUALTEST
  DEPENDS Utils QtcSsh Qt5::Widgets
  INCLUDES "${PROJECT_SOURCE_DIR}/src/shared/modeltest"
  DEFINES "TEST_RELATIVE_LIBEXEC_PATH=\"${TEST_RELATIVE_LIBEXEC_PATH}\""
  SOURCES
    main.cpp
    window.cpp window.h window.ui
)

extend_qtc_test(tst_manual_sftpfsmodel
  SOURCES_PREFIX "${PROJECT_SOURCE_DIR}/src/shared/modeltest/"
  SOURCES
    modeltest.cpp modeltest.h
)
