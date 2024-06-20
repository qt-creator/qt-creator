add_qtc_library(CmdBridgeClient
  CONDITION TARGET CmdBridge
  PUBLIC_DEPENDS Utils
  DEFINES GOBRIDGE_MAGIC_PACKET_MARKER=\"${GOBRIDGE_MAGIC_PACKET_MARKER}\"
  SOURCES
    bridgedfileaccess.cpp
    bridgedfileaccess.h
    cmdbridgeclient.cpp
    cmdbridgeclient.h
    cmdbridgeglobal.h
    cmdbridgetr.h
)

if (TARGET CmdBridge)
  add_dependencies(CmdBridgeClient CmdBridge)
endif()
