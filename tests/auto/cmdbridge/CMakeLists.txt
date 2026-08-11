add_qtc_test(tst_cmdbridge
  CONDITION TARGET CmdBridgeClient
  TIMEOUT 600
  DEFINES "TEST_LIBEXEC_PATH=\"${CMAKE_BINARY_DIR}/${IDE_LIBEXEC_PATH}\""
  DEPENDS CmdBridgeClient app_version Qt::Network
  SOURCES
    tst_cmdbridge.cpp
)
