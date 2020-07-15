cmake_minimum_required(VERSION 3.10)

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
endif()

configure_file(../../app/app_version.h.cmakein app/app_version.h ESCAPE_QUOTES)

find_package(Qt5
  COMPONENTS Core
  REQUIRED
)

if (MSVC)
  # find out if Qt is static and set /MT if so
  get_target_property(_input_type Qt5::Core TYPE)
  if (${_input_type} STREQUAL "STATIC_LIBRARY")
    string(REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
    string(REPLACE "/MDd" "/MTd" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
  endif()
endif()

if (APPLE)
  find_library(FWFoundation Foundation)
endif()

set(UtilsSourcesDir "../../libs/utils")

add_qtc_executable(sdktool
  ${DESTINATION}
  DEFINES DATA_PATH=\"${IDE_DATA_PATH}\"
  DEPENDS Qt5::Core
  INCLUDES
    "${UtilsSourcesDir}/../"
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
    main.cpp
    operation.cpp operation.h
    rmcmakeoperation.cpp rmcmakeoperation.h
    rmdebuggeroperation.cpp rmdebuggeroperation.h
    rmdeviceoperation.cpp rmdeviceoperation.h
    rmkeysoperation.cpp rmkeysoperation.h
    rmkitoperation.cpp rmkitoperation.h
    rmqtoperation.cpp rmqtoperation.h
    rmtoolchainoperation.cpp rmtoolchainoperation.h
    settings.cpp settings.h
)

extend_qtc_executable(sdktool
  SOURCES_PREFIX "${UtilsSourcesDir}"
  DEFINES QTCREATOR_UTILS_STATIC_LIB
  SOURCES
    environment.cpp environment.h
    fileutils.cpp fileutils.h
    hostosinfo.cpp hostosinfo.h
    namevaluedictionary.cpp namevaluedictionary.h
    namevalueitem.cpp namevalueitem.h
    persistentsettings.cpp persistentsettings.h
    qtcassert.cpp qtcassert.h
    qtcprocess.cpp qtcprocess.h
    savefile.cpp savefile.h
    stringutils.cpp stringutils.h
)

extend_qtc_executable(sdktool CONDITION APPLE
  SOURCES_PREFIX "${UtilsSourcesDir}"
  SOURCES
    fileutils_mac.mm fileutils_mac.h
  DEPENDS
    ${FWFoundation}
)

extend_qtc_executable(sdktool CONDITION WIN32
  DEPENDS
    user32 iphlpapi ws2_32 shell32
  DEFINES
    _UNICODE UNICODE
    _CRT_SECURE_NO_WARNINGS _SCL_SECURE_NO_WARNINGS
)
