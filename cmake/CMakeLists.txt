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
      EXCLUDE_FROM_ALL
      ${exclusion_mask}
    )
    list(APPEND QT_PLUGIN_DIRECTORIES "${QT_DEST_PLUGIN_PATH}/${plugin}")
  endforeach()

  install(
    DIRECTORY "${QT_BASE_DIR}/qml/"
    DESTINATION ${QT_DEST_QML_PATH}
    COMPONENT Dependencies
    EXCLUDE_FROM_ALL
    PATTERN "qml/*"
    ${exclusion_mask}
  )

  # QtCreator's "System Information..." needs qtdiag
  set(qtdiag_destination ${IDE_BIN_PATH})
  if (NOT APPLE AND NOT WIN32)
    set(qtdiag_destination ${IDE_LIBRARY_BASE_PATH}/Qt/bin)
  endif()
  install(PROGRAMS
    "${QT_BASE_DIR}/bin/qtdiag${CMAKE_EXECUTABLE_SUFFIX}"
    DESTINATION ${qtdiag_destination}
    COMPONENT Dependencies
    EXCLUDE_FROM_ALL
  )

  # Analyze the binaries and install missing dependencies if they are
  # found the CMAKE_PREFIX_PATH e.g. Qt, Clang
  configure_file(InstallDependentSharedObjects.cmake.in InstallDependentSharedObjects.cmake @ONLY)
  install(
    SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/InstallDependentSharedObjects.cmake
    COMPONENT Dependencies
    EXCLUDE_FROM_ALL
  )

  if (MSVC)
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP ON)
    include(InstallRequiredsystemLibraries)

    # For Qt Creator
    install(PROGRAMS ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS}
      DESTINATION ${IDE_APP_PATH}
      COMPONENT Dependencies
      EXCLUDE_FROM_ALL
    )

    # For qtcreatorcdbext
    set(ArchSuffix 32)
    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
      set(ArchSuffix 64)
    endif()

    install(PROGRAMS ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS}
      DESTINATION lib/qtcreatorcdbext${ArchSuffix}
      COMPONENT Dependencies
      EXCLUDE_FROM_ALL
    )
  endif()
endif()
