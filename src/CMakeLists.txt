add_library(app_version INTERFACE)
target_include_directories(app_version
  INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
  INTERFACE $<INSTALL_INTERFACE:${IDE_HEADER_INSTALL_PATH}/src>)
install(TARGETS app_version EXPORT QtCreator)

add_subdirectory(libs)
add_subdirectory(share)
add_subdirectory(shared)
add_subdirectory(plugins)
add_subdirectory(app)
add_subdirectory(tools)

install(
  FILES
    ${PROJECT_SOURCE_DIR}/HACKING
    ${PROJECT_SOURCE_DIR}/LICENSE.GPL3-EXCEPT
    ${PROJECT_SOURCE_DIR}/README.md
  DESTINATION ${IDE_DATA_PATH}
  COMPONENT Devel EXCLUDE_FROM_ALL
)

install(
  DIRECTORY
    ${PROJECT_SOURCE_DIR}/scripts
  DESTINATION ${IDE_DATA_PATH}
  COMPONENT Devel EXCLUDE_FROM_ALL
  USE_SOURCE_PERMISSIONS
)

install(EXPORT QtCreator
  DESTINATION ${IDE_CMAKE_INSTALL_PATH}/QtCreator
  COMPONENT Devel EXCLUDE_FROM_ALL
  NAMESPACE QtCreator::
  FILE QtCreatorTargets.cmake
)

file(WRITE ${CMAKE_BINARY_DIR}/cmake/QtCreatorConfig.cmake "
\# force plugins to same path naming conventions as Qt Creator
\# otherwise plugins will not be found
if(UNIX AND NOT APPLE)
  include(GNUInstallDirs)
  set(CMAKE_INSTALL_BINDIR ${CMAKE_INSTALL_BINDIR})
  set(CMAKE_INSTALL_LIBDIR ${CMAKE_INSTALL_LIBDIR})
  set(CMAKE_INSTALL_LIBEXECDIR ${CMAKE_INSTALL_LIBEXECDIR})
  set(CMAKE_INSTALL_DATAROOTDIR ${CMAKE_INSTALL_DATAROOTDIR})
endif()

include(CMakeFindDependencyMacro)
find_dependency(Qt6 ${IDE_QT_VERSION_MIN}
  COMPONENTS Concurrent Core Gui Widgets Core5Compat Network PrintSupport Qml Sql REQUIRED
)
find_dependency(Qt6 COMPONENTS Quick QuickWidgets QUIET)

if (NOT IDE_VERSION)
  include(\${CMAKE_CURRENT_LIST_DIR}/QtCreatorIDEBranding.cmake)
endif()

if (NOT DEFINED add_qtc_plugin)
  include(\${CMAKE_CURRENT_LIST_DIR}/QtCreatorAPI.cmake)
endif()

if (NOT DEFINED add_translation_targets)
  include(\${CMAKE_CURRENT_LIST_DIR}/QtCreatorTranslations.cmake)
endif()

if (NOT DEFINED add_qtc_documentation)
  include(\${CMAKE_CURRENT_LIST_DIR}/QtCreatorDocumentation.cmake)
endif()

if (NOT TARGET QtCreator::Core)
  include(\${CMAKE_CURRENT_LIST_DIR}/QtCreatorTargets.cmake)
endif()
")

# Local build export
export(EXPORT QtCreator
  NAMESPACE QtCreator::
  FILE ${CMAKE_BINARY_DIR}/cmake/QtCreatorTargets.cmake)

file(COPY
  ${IDE_BRANDING_FILE}
  ${PROJECT_SOURCE_DIR}/cmake/QtCreatorTranslations.cmake
  ${PROJECT_SOURCE_DIR}/cmake/QtCreatorDocumentation.cmake
  ${PROJECT_SOURCE_DIR}/cmake/QtCreatorAPI.cmake
  ${PROJECT_SOURCE_DIR}/cmake/QtCreatorAPIInternal.cmake
  ${PROJECT_SOURCE_DIR}/cmake/Config.cmake.in
  ${PROJECT_SOURCE_DIR}/cmake/QtcSeparateDebugInfo.cmake
  ${PROJECT_SOURCE_DIR}/cmake/QtcSeparateDebugInfo.Info.plist.in
  DESTINATION ${CMAKE_BINARY_DIR}/cmake
)

# Devel package install
install(
  FILES
    ${IDE_BRANDING_FILE}
    ${PROJECT_SOURCE_DIR}/cmake/QtCreatorTranslations.cmake
    ${PROJECT_SOURCE_DIR}/cmake/QtCreatorDocumentation.cmake
    ${PROJECT_SOURCE_DIR}/cmake/QtCreatorAPI.cmake
    ${PROJECT_SOURCE_DIR}/cmake/QtCreatorAPIInternal.cmake
    ${PROJECT_SOURCE_DIR}/cmake/Config.cmake.in
    ${PROJECT_SOURCE_DIR}/cmake/QtcSeparateDebugInfo.cmake
    ${PROJECT_SOURCE_DIR}/cmake/QtcSeparateDebugInfo.Info.plist.in
    ${CMAKE_BINARY_DIR}/cmake/QtCreatorConfig.cmake
  DESTINATION ${IDE_CMAKE_INSTALL_PATH}/QtCreator
  COMPONENT Devel EXCLUDE_FROM_ALL
)
