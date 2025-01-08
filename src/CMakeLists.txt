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
    ${PROJECT_SOURCE_DIR}/LICENSES/LICENSE.GPL3-EXCEPT
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

include(CMakePackageConfigHelpers)

set(configure_package_config_file_input_path "${PROJECT_SOURCE_DIR}/cmake/QtCreatorConfig.cmake.in")
set(common_configure_package_config_file_options
  NO_SET_AND_CHECK_MACRO
  NO_CHECK_REQUIRED_COMPONENTS_MACRO
  PATH_VARS
    QTCREATOR_RELOCATABLE_INSTALL_PREFIX
)

# Build-tree Config file, used in super build projects that call add_subdirectory(qtcreator).
set(QTCREATOR_RELOCATABLE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}")
configure_package_config_file(
  "${configure_package_config_file_input_path}"
  "${CMAKE_BINARY_DIR}/cmake/QtCreatorConfig.cmake"
  INSTALL_DESTINATION "${CMAKE_BINARY_DIR}/cmake"
  INSTALL_PREFIX "${CMAKE_BINARY_DIR}"
  ${common_configure_package_config_file_options}
)

# Install-tree Config file, used in non-super build projects.
set(QTCREATOR_RELOCATABLE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")
configure_package_config_file(
  "${configure_package_config_file_input_path}"
  # This is generated into a different directory on purpose, so it doesn't collide with the
  # build-tree config file.
  "${CMAKE_BINARY_DIR}/cmake/install/QtCreatorConfig.cmake"
  INSTALL_DESTINATION "${IDE_CMAKE_INSTALL_PATH}/QtCreator"
  ${common_configure_package_config_file_options}
)

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
  ${PROJECT_SOURCE_DIR}/cmake/QtCreatorSbom.cmake
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
    ${PROJECT_SOURCE_DIR}/cmake/QtCreatorSbom.cmake
    ${PROJECT_SOURCE_DIR}/cmake/Config.cmake.in
    ${PROJECT_SOURCE_DIR}/cmake/QtcSeparateDebugInfo.cmake
    ${PROJECT_SOURCE_DIR}/cmake/QtcSeparateDebugInfo.Info.plist.in
    ${CMAKE_BINARY_DIR}/cmake/install/QtCreatorConfig.cmake
  DESTINATION ${IDE_CMAKE_INSTALL_PATH}/QtCreator
  COMPONENT Devel EXCLUDE_FROM_ALL
)
