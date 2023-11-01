cmake_minimum_required(VERSION 3.16)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../cmake")

project(sdktool)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (NOT QT_CREATOR_API_DEFINED)
  # standalone build
  set(DESTINATION DESTINATION .)
  include(QtCreatorIDEBranding)
  include(QtCreatorAPI)
  qtc_handle_compiler_cache_support()

  find_package(QT NAMES Qt6 Qt5
    COMPONENTS Core
    REQUIRED
  )
  find_package(Qt${QT_VERSION_MAJOR}
    COMPONENTS Core
    REQUIRED
  )
endif()

configure_file(../../app/app_version.h.cmakein app/app_version.h ESCAPE_QUOTES)

if (APPLE)
  find_library(FWFoundation Foundation)
endif()

set(UtilsSourcesDir "../../libs/utils")

file(RELATIVE_PATH DATA_PATH "/${IDE_LIBEXEC_PATH}" "/${IDE_DATA_PATH}")

add_qtc_library(sdktoolLib
  STATIC
  DEFINES DATA_PATH=\"${DATA_PATH}\"
  PUBLIC_DEPENDS Qt::Core
  PUBLIC_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR} "${UtilsSourcesDir}/../"
  INCLUDES
    ${CMAKE_CURRENT_BINARY_DIR}
  SOURCES
    addabiflavor.cpp addabiflavor.h
    addcmakeoperation.cpp addcmakeoperation.h
    adddebuggeroperation.cpp adddebuggeroperation.h
    adddeviceoperation.cpp adddeviceoperation.h
    addkeysoperation.cpp addkeysoperation.h
    addkitoperation.cpp addkitoperation.h
    addqtoperation.cpp addqtoperation.h
    addtoolchainoperation.cpp addtoolchainoperation.h
    addvalueoperation.cpp addvalueoperation.h
    findkeyoperation.cpp findkeyoperation.h
    findvalueoperation.cpp findvalueoperation.h
    getoperation.cpp getoperation.h
    operation.cpp operation.h
    rmcmakeoperation.cpp rmcmakeoperation.h
    rmdebuggeroperation.cpp rmdebuggeroperation.h
    rmdeviceoperation.cpp rmdeviceoperation.h
    rmkeysoperation.cpp rmkeysoperation.h
    rmkitoperation.cpp rmkitoperation.h
    rmqtoperation.cpp rmqtoperation.h
    rmtoolchainoperation.cpp rmtoolchainoperation.h
    settings.cpp settings.h
    sdkpersistentsettings.cpp sdkpersistentsettings.h
)

if (MSVC)
    find_library(DbgHelpLib dbghelp)
endif()

extend_qtc_library(sdktoolLib CONDITION MSVC
  DEPENDS ${DbgHelpLib}
)
extend_qtc_library(sdktoolLib CONDITION WIN32
  PUBLIC_DEPENDS
    user32 iphlpapi ws2_32 shell32
  DEFINES
    _UNICODE UNICODE
    _CRT_SECURE_NO_WARNINGS _SCL_SECURE_NO_WARNINGS
)

add_qtc_executable(sdktool
  ${DESTINATION}
  DEFINES DATA_PATH=\"${DATA_PATH}\"
  DEPENDS sdktoolLib
  INCLUDES
    ${CMAKE_CURRENT_BINARY_DIR}
  SOURCES
    main.cpp
)

if (MSVC AND TARGET sdktool AND TARGET Qt5::Core)
  # find out if Qt is static and set /MT if so
  get_target_property(_input_type Qt5::Core TYPE)
  if (${_input_type} STREQUAL "STATIC_LIBRARY")
    set_property(TARGET sdktool PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    set_property(TARGET sdktoolLib PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  endif()
endif()

