add_qtc_test(tst_manual_sourceprofiletransfer
  MANUALTEST
  NEEDS_GUI
  CONDITION TARGET Remote
  DEFINES "QTC_TEST_LIBEXEC_PATH=\"${CMAKE_BINARY_DIR}/${IDE_LIBEXEC_PATH}\""
  DEPENDS Remote ProjectExplorer Core Utils
  SOURCES
    tst_sourceprofiletransfer.cpp
)
