set(DEBUGGERDIR "${PROJECT_SOURCE_DIR}/src/plugins/debugger")

add_qtc_test(tst_debugger_disassembler
  INCLUDES "${DEBUGGERDIR}"
  SOURCES
    tst_disassembler.cpp
    "${DEBUGGERDIR}/disassemblerlines.cpp" "${DEBUGGERDIR}/disassemblerlines.h"
)

option(WITH_DEBUGGER_DUMPERS "Include tests for debugger pretty printers" ON)
if (WITH_DEBUGGER_DUMPERS)
  get_target_property(qmake_binary Qt5::qmake IMPORTED_LOCATION)

  add_qtc_test(tst_debugger_dumpers
    TIMEOUT 0
    DEPENDS Qt5::Network Utils
    DEFINES
      DUMPERDIR="${PROJECT_SOURCE_DIR}/share/qtcreator/debugger"
      DEFAULT_QMAKE_BINARY="${qmake_binary}"
    INCLUDES
      "${DEBUGGERDIR}"
    SOURCES
      "${DEBUGGERDIR}/debuggerprotocol.cpp" "${DEBUGGERDIR}/debuggerprotocol.h"
      "${DEBUGGERDIR}/simplifytype.cpp" "${DEBUGGERDIR}/simplifytype.h"
      "${DEBUGGERDIR}/watchdata.cpp" "${DEBUGGERDIR}/watchdata.h"
      "${DEBUGGERDIR}/watchutils.cpp" "${DEBUGGERDIR}/watchutils.h"
      tst_dumpers.cpp
  )
endif()

add_qtc_test(tst_debugger_gdb
  DEPENDS Qt5::Network Utils
  INCLUDES "${DEBUGGERDIR}"
  SOURCES
    tst_gdb.cpp
    "${DEBUGGERDIR}/debuggerprotocol.cpp" "${DEBUGGERDIR}/debuggerprotocol.h"
)

add_qtc_test(tst_debugger_protocol
  DEPENDS Qt5::Network Utils
  INCLUDES
    "${DEBUGGERDIR}"
  SOURCES
    "${DEBUGGERDIR}/debuggerprotocol.cpp" "${DEBUGGERDIR}/debuggerprotocol.h"
    tst_protocol.cpp
)

add_qtc_test(tst_debugger_offsets
  DEPENDS Qt5::CorePrivate
  INCLUDES "${DEBUGGERDIR}"
  SOURCES tst_offsets.cpp
)

add_qtc_test(tst_debugger_simplifytypes
  INCLUDES "${DEBUGGERDIR}"
  DEPENDS Utils
  DEFINES DUMPERDIR="${PROJECT_SOURCE_DIR}/share/qtcreator/debugger"
  SOURCES
    tst_simplifytypes.cpp
    "${DEBUGGERDIR}/simplifytype.cpp" "${DEBUGGERDIR}/simplifytype.h"
)
