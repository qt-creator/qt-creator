add_qtc_test(tst_manual_sourceprofiletransfer
  MANUALTEST
  NEEDS_GUI
  CONDITION TARGET RemoteLinux
  DEFINES "QTC_TEST_LIBEXEC_PATH=\"${CMAKE_BINARY_DIR}/${IDE_LIBEXEC_PATH}\""
  DEPENDS RemoteLinux ProjectExplorer Core Utils
  SOURCES
    tst_sourceprofiletransfer.cpp
)
