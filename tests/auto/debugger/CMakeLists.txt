cmake_minimum_required(VERSION 3.16)
if (NOT QT_CREATOR_API_DEFINED)
  list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../cmake")

  project(debugger)

  enable_testing()

  # Needed for pch
  set(QtCreator_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../")
  option(BUILD_WITH_PCH "Build with precompiled headers" ON)

  set(PROJECT_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../")

  set(CMAKE_AUTOMOC ON)
  set(CMAKE_AUTORCC ON)
  set(CMAKE_AUTOUIC ON)

  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)

  set(IMPLICIT_DEPENDS Qt::Test)

  include(QtCreatorIDEBranding)
  include(QtCreatorAPI)
  qtc_handle_compiler_cache_support()

  set(WITH_TESTS ON)

  find_package(Qt6
    COMPONENTS
      Gui Core Core5Compat Widgets Network Qml Concurrent Test Xml MODULE)
  find_package(Threads)

  add_subdirectory(${QtCreator_SOURCE_DIR}/src/libs/utils ${CMAKE_CURRENT_BINARY_DIR}/utils)
endif()

set(DEBUGGERDIR "${PROJECT_SOURCE_DIR}/src/plugins/debugger")

add_qtc_test(tst_debugger_disassembler
  INCLUDES "${DEBUGGERDIR}"
  SOURCES
    tst_disassembler.cpp
    "${DEBUGGERDIR}/disassemblerlines.cpp" "${DEBUGGERDIR}/disassemblerlines.h"
)

option(WITH_DEBUGGER_DUMPERS "Include tests for debugger pretty printers" ON)
if (WITH_DEBUGGER_DUMPERS)
  get_target_property(qmake_binary Qt::qmake IMPORTED_LOCATION)

  add_qtc_test(tst_debugger_dumpers
    EXCLUDE_FROM_PRECHECK
    TIMEOUT 0
    DEPENDS Qt::Network Utils
    DEFINES
      DUMPERDIR="${PROJECT_SOURCE_DIR}/share/qtcreator/debugger"
      DEFAULT_QMAKE_BINARY="${qmake_binary}"
    INCLUDES
      "${DEBUGGERDIR}"
    SOURCES
      "${DEBUGGERDIR}/debuggertr.h"
      "${DEBUGGERDIR}/debuggerprotocol.cpp" "${DEBUGGERDIR}/debuggerprotocol.h"
      "${DEBUGGERDIR}/simplifytype.cpp" "${DEBUGGERDIR}/simplifytype.h"
      "${DEBUGGERDIR}/watchdata.cpp" "${DEBUGGERDIR}/watchdata.h"
      "${DEBUGGERDIR}/watchutils.cpp" "${DEBUGGERDIR}/watchutils.h"
      tst_dumpers.cpp
  )
endif()

add_qtc_test(tst_debugger_gdb
  DEPENDS Qt::Network Utils
  INCLUDES "${DEBUGGERDIR}"
  SOURCES
    tst_gdb.cpp
    "${DEBUGGERDIR}/debuggertr.h"
    "${DEBUGGERDIR}/debuggerprotocol.cpp" "${DEBUGGERDIR}/debuggerprotocol.h"
)

add_qtc_test(tst_debugger_protocol
  DEPENDS Qt::Network Utils
  INCLUDES
    "${DEBUGGERDIR}"
  SOURCES
    "${DEBUGGERDIR}/debuggertr.h"
    "${DEBUGGERDIR}/debuggerprotocol.cpp" "${DEBUGGERDIR}/debuggerprotocol.h"
    tst_protocol.cpp
)

add_qtc_test(tst_debugger_offsets
  DEPENDS Qt::CorePrivate
  INCLUDES "${DEBUGGERDIR}"
  SOURCES tst_offsets.cpp
)

add_qtc_test(tst_debugger_simplifytypes
  INCLUDES "${DEBUGGERDIR}"
  DEPENDS Utils
  DEFINES DUMPERDIR="${PROJECT_SOURCE_DIR}/share/qtcreator/debugger"
  SOURCES
    tst_simplifytypes.cpp
    "${DEBUGGERDIR}/debuggertr.h"
    "${DEBUGGERDIR}/simplifytype.cpp" "${DEBUGGERDIR}/simplifytype.h"
)
