cmake_minimum_required(VERSION 3.16)

# standalone build
if (NOT QT_CREATOR_API_DEFINED)
  list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../cmake")

  project(sqlitetester)

  set(CMAKE_AUTOMOC ON)
  set(CMAKE_AUTORCC ON)
  set(CMAKE_AUTOUIC ON)
  set(CMAKE_CXX_STANDARD 20)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)

  set(DESTINATION DESTINATION .)
  include(QtCreatorIDEBranding)
  include(QtCreatorAPI)
  qtc_handle_compiler_cache_support()

  find_package(Qt6
    COMPONENTS Core Gui Widgets
    REQUIRED
  )
endif()

add_qtc_executable(sqlitetester
  ${DESTINATION}
  CONDITION WINDOWS AND BUILD_DESIGNSTUDIO
  DEPENDS
    Sqlite
    Qt::Core Qt::Widgets
  INCLUDES
    ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}
  SOURCES
    main.cpp
)
