# Deployment of Qt, Clang, C++ Runtime libraries is being done in this script.
#
# It should be included as a subdirectory as last because of CMake's
# cmake_install.cmake script behviour of including subdirectories at the end
# of the script, not respecting the order of install commands from CMakeLists.txt
#
# This way we are sure that all the binaries have been installed before.

if (CMAKE_VERSION VERSION_GREATER_EQUAL 3.16)
  get_target_property(moc_binary Qt5::moc IMPORTED_LOCATION)
  get_filename_component(moc_dir "${moc_binary}" DIRECTORY)
  get_filename_component(QT_BASE_DIR "${moc_dir}/../" ABSOLUTE)

  if (MSVC AND NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(exclusion_mask PATTERN "*d.dll" EXCLUDE)
  endif()

  foreach(plugin
    assetimporters designer iconengines imageformats platforminputcontexts
    platforms platformthemes printsupport qmltooling sqldrivers styles
    xcbglintegrations)
    if(NOT EXISTS "${QT_BASE_DIR}/plugins/${plugin}")
      continue()
    endif()
    install(
      DIRECTORY "${QT_BASE_DIR}/plugins/${plugin}"
      DESTINATION ${QT_DEST_PLUGIN_PATH}
      COMPONENT Dependencies
      ${exclusion_mask}
    )
    list(APPEND QT_PLUGIN_DIRECTORIES "${QT_DEST_PLUGIN_PATH}/${plugin}")
  endforeach()

  install(
    DIRECTORY "${QT_BASE_DIR}/qml/"
    DESTINATION ${QT_DEST_QML_PATH}
    COMPONENT Dependencies
    PATTERN "qml/*"
    ${exclusion_mask}
  )

  # Analyze the binaries and install missing dependencies if they are
  # found the CMAKE_PREFIX_PATH e.g. Qt, Clang
  configure_file(InstallDependentSharedObjects.cmake.in InstallDependentSharedObjects.cmake @ONLY)
  install(
    SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/InstallDependentSharedObjects.cmake
    COMPONENT Dependencies
  )

  if (MSVC)
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP ON)
    include(InstallRequiredsystemLibraries)

    install(PROGRAMS ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS}
      DESTINATION ${IDE_APP_PATH}
      COMPONENT Dependencies
    )
  endif()
endif()
