cmake_minimum_required(VERSION 3.20)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../cmake")

project(AppStatisticMonitor)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 COMPONENTS Charts QUIET)

set(APPSTATISTICSMONITOR_DISABLEDBYDEFAULT "true")

if (NOT QT_CREATOR_API_DEFINED)
  # standalone build
  set(DESTINATION DESTINATION .)
  include(QtCreatorIDEBranding)
  include(QtCreatorAPI)
  qtc_handle_compiler_cache_support()

  find_package(QtCreator COMPONENTS Core ProjectExplorer Utils REQUIRED)

  set(APPSTATISTICSMONITOR_DISABLEDBYDEFAULT "false")
endif()


add_qtc_plugin(AppStatisticsMonitor
  CONDITION TARGET Qt6::Charts
  SKIP_TRANSLATION
  DEPENDS Qt6::Charts QtCreator::Utils
  PLUGIN_DEPENDS QtCreator::Core QtCreator::ProjectExplorer
  SOURCES
    appstatisticsmonitorplugin.cpp
    chart.cpp chart.h
    manager.cpp manager.h
    idataprovider.h idataprovider.cpp
)

