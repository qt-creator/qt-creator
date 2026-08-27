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

  set(CMAKE_CXX_STANDARD 20)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)

  set(IMPLICIT_DEPENDS Qt::Test)

  include(QtCreatorIDEBranding)
  include(QtCreatorAPI)
  qtc_handle_compiler_cache_support()

  set(WITH_TESTS ON)

  find_package(Qt6
    COMPONENTS
      Gui Core Widgets Network Qml Concurrent Test Xml
    OPTIONAL_COMPONENTS
      Quick
    MODULE)
  find_package(Threads)

  add_subdirectory(${QtCreator_SOURCE_DIR}/src/libs/utils ${CMAKE_CURRENT_BINARY_DIR}/utils)
endif()

set(DEBUGGERDIR "${PROJECT_SOURCE_DIR}/src/plugins/debugger")

include(CheckCXXSourceCompiles)
set(CMAKE_REQUIRED_LIBRARIES "") # Ensure no external link dependencies interfere with the probe
check_cxx_source_compiles("
    typedef __int128_t int128_t;
    int main() {
        volatile int128_t v = 12345678901234567890ULL;
        unsigned int base = 10;
        unsigned int rem = (unsigned int)(v % base);
        v = v / base;
        (void)rem;
        (void)v;
        return 0;
    }
" HAVE_INT128)

if(HAVE_INT128)
  add_definitions(-DHAVE_INT128)
endif()

add_qtc_test(tst_debugger_disassembler
  INCLUDES "${DEBUGGERDIR}"
  SOURCES
    tst_disassembler.cpp
    "${DEBUGGERDIR}/disassemblerlines.cpp" "${DEBUGGERDIR}/disassemblerlines.h"
)

option(WITH_DEBUGGER_DUMPERS "Include tests for debugger pretty printers" ON)
if (WITH_DEBUGGER_DUMPERS)
  get_target_property(qmake_binary Qt::qmake IMPORTED_LOCATION)

  if (QtCreator_BINARY_DIR)
    set(CDBEXT_BASE_PATH "${QtCreator_BINARY_DIR}/${IDE_LIBRARY_BASE_PATH}")
  else()
    set(CDBEXT_BASE_PATH "${CMAKE_BINARY_DIR}/${IDE_LIBRARY_BASE_PATH}")
  endif()

  add_qtc_test(tst_debugger_dumpers
    EXCLUDE_FROM_PRECHECK
    TIMEOUT 0
    DEPENDS Qt::Network Utils Debugger
    DEFINES
      DUMPERDIR="${PROJECT_SOURCE_DIR}/share/qtcreator/debugger"
      CDBEXT_PATH="${CDBEXT_BASE_PATH}"
      DEFAULT_QMAKE_BINARY="${qmake_binary}"
    INCLUDES
      "${DEBUGGERDIR}"
    SOURCES
      "${DEBUGGERDIR}/debuggertr.h"
      "${DEBUGGERDIR}/debuggerprotocol.cpp" "${DEBUGGERDIR}/debuggerprotocol.h"
      "${DEBUGGERDIR}/simplifytype.cpp" "${DEBUGGERDIR}/simplifytype.h"
      "${DEBUGGERDIR}/watchutils.cpp" "${DEBUGGERDIR}/watchutils.h"
      tst_dumpers.cpp
  )
endif()

# Off by default: needs a real gdb/lldb plus a Qt with QML debugging and the
# qmldbg_native plugin, so it is not run automatically on CI. Enable explicitly.
option(WITH_DEBUGGER_NATIVE_MIXED "Include test for native combined C++/QML debugging" OFF)
if (WITH_DEBUGGER_NATIVE_MIXED)
  get_target_property(qmake_binary Qt::qmake IMPORTED_LOCATION)

  add_qtc_test(tst_debugger_nativemixed
    EXCLUDE_FROM_PRECHECK
    TIMEOUT 0
    DEPENDS Qt::Test
    DEFINES
      DUMPERDIR="${PROJECT_SOURCE_DIR}/share/qtcreator/debugger"
      DEFAULT_QMAKE_BINARY="${qmake_binary}"
      NATIVEMIXED_DRIVER="${CMAKE_CURRENT_SOURCE_DIR}/nativemixed_driver.py"
      QMLMIX_SOURCE_DIR="${PROJECT_SOURCE_DIR}/tests/manual/debugger/qmlmix"
    SOURCES
      tst_nativemixed.cpp
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

set(BACKENDS_TEST_DEFINES DUMPERDIR="${PROJECT_SOURCE_DIR}/share/qtcreator/debugger")
list(APPEND BACKENDS_TEST_DEFINES
  "BACKENDS_TEST_SOURCE_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}\"")

if (TARGET Qt::Quick)
  qt_add_executable(qmlstack_inferior
    qmlstack_inferior.cpp qmlstack_inferior.qrc
  )
  target_link_libraries(qmlstack_inferior PRIVATE Qt::Quick Qt::Qml)
  target_compile_definitions(qmlstack_inferior PRIVATE QT_QML_DEBUG)
  list(APPEND BACKENDS_TEST_DEFINES
    "QMLSTACK_INFERIOR_EXECUTABLE=\"$<TARGET_FILE_DIR:qmlstack_inferior>\"")

  qt_add_executable(qmlserver_inferior
    qmlserver_inferior.cpp qmlserver_inferior.qrc
  )
  target_link_libraries(qmlserver_inferior PRIVATE Qt::Quick Qt::Qml)
  target_compile_definitions(qmlserver_inferior PRIVATE QT_QML_DEBUG)
  list(APPEND BACKENDS_TEST_DEFINES
    "QMLSERVER_INFERIOR_EXECUTABLE=\"$<TARGET_FILE_DIR:qmlserver_inferior>\"")

  qt_add_executable(qmlmix_inferior qmlmix_inferior.cpp)
  qt_add_qml_module(qmlmix_inferior
    URI MixTest
    VERSION 1.0
    RESOURCE_PREFIX "/qt/qml"
    QML_FILES Main.qml
    SOURCES qmlmix_inferior.h
  )
  target_compile_definitions(qmlmix_inferior PRIVATE QT_QML_DEBUG)
  target_link_libraries(qmlmix_inferior PRIVATE Qt::Quick)
  list(APPEND BACKENDS_TEST_DEFINES
    "QMLMIX_INFERIOR_EXECUTABLE=\"$<TARGET_FILE_DIR:qmlmix_inferior>\"")
endif()

add_qtc_test(tst_debugger_backends
  TIMEOUT 0
  DEPENDS Qt::Network Utils Debugger
  DEFINES ${BACKENDS_TEST_DEFINES}
  INCLUDES
    "${DEBUGGERDIR}"
  SOURCES
    "${DEBUGGERDIR}/debuggerprotocol.cpp" "${DEBUGGERDIR}/debuggerprotocol.h"
    "${DEBUGGERDIR}/disassemblerlines.cpp" "${DEBUGGERDIR}/disassemblerlines.h"
    tst_backends.cpp
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

add_qtc_test(tst_debugger_pdb
  DEFINES
    DUMPERDIR="${PROJECT_SOURCE_DIR}/share/qtcreator/debugger"
    PDBDUMPER_DRIVER="${CMAKE_CURRENT_SOURCE_DIR}/pdbdumper.py"
  SOURCES
    tst_pdb.cpp
)
