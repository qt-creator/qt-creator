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
  get_filename_component(qt5_base_dir "${moc_dir}/../" ABSOLUTE)

  if (MSVC AND NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(exclusion_mask PATTERN "*d.dll" EXCLUDE)
  endif()

  if (WIN32)
    set(qt5_plugin_dest_dir ${IDE_BIN_PATH}/plugins)
    set(qt5_qml_dest_dir ${IDE_BIN_PATH}/qml)
  elseif(APPLE)
    set(qt5_plugin_dest_dir ${IDE_PLUGIN_PATH})
    set(qt5_qml_dest_dir ${IDE_DATA_PATH}/../Imports/qtquick2)
  else()
    set(qt5_plugin_dest_dir ${IDE_LIBRARY_BASE_PATH}/Qt/plugins)
    set(qt5_qml_dest_dir ${IDE_LIBRARY_BASE_PATH}/Qt/qml)
  endif()

  foreach(plugin
    designer iconengines imageformats platforms platformthemes
    printsupport qmltooling sqldrivers styles)
    if(NOT EXISTS "${qt5_base_dir}/plugins/${plugin}")
      continue()
    endif()
    install(
      DIRECTORY "${qt5_base_dir}/plugins/${plugin}"
      DESTINATION ${qt5_plugin_dest_dir}
      ${exclusion_mask}
    )
    list(APPEND qt5_plugin_directories "${qt5_plugin_dest_dir}/${plugin}")
  endforeach()

  install(
    DIRECTORY "${qt5_base_dir}/qml/"
    DESTINATION ${qt5_qml_dest_dir}
    PATTERN "qml/*"
    ${exclusion_mask}
  )

  install(CODE "
    function(create_qt_conf location base_dir)
      get_filename_component(install_prefix \"\${CMAKE_INSTALL_PREFIX}\" ABSOLUTE)
      file(RELATIVE_PATH qt_conf_binaries
        \"\${install_prefix}/\${base_dir}\"
        \"\${install_prefix}/${IDE_BIN_PATH}\"
      )
      if (NOT qt_conf_binaries)
        set(qt_conf_binaries .)
      endif()
      file(RELATIVE_PATH qt_conf_plugins
        \"\${install_prefix}/\${base_dir}\"
        \"\${install_prefix}/${qt5_plugin_dest_dir}\"
      )
      file(RELATIVE_PATH qt_conf_qml
        \"\${install_prefix}/\${base_dir}\"
        \"\${install_prefix}/${qt5_qml_dest_dir}\"
      )

      file(WRITE \"\${CMAKE_INSTALL_PREFIX}/\${location}/qt.conf\"
        \"[Paths]\n\"
        \"Binaries=\${qt_conf_binaries}\n\"
        \"Plugins=\${qt_conf_plugins}\n\"
        \"Qml2Imports=\${qt_conf_qml}\n\"
      )
    endfunction()

    if(APPLE)
      create_qt_conf(\"${IDE_DATA_PATH}\" \"${IDE_DATA_PATH}/..\")
      create_qt_conf(\"${IDE_LIBEXEC_PATH}\" \"${IDE_DATA_PATH}/..\")
    else()
      create_qt_conf(\"${IDE_APP_PATH}\" \"${IDE_APP_PATH}\")
      create_qt_conf(\"${IDE_LIBEXEC_PATH}\" \"${IDE_LIBEXEC_PATH}\")
    endif()

    "
   )

  # Analyze the binaries and install missing dependencies if they are
  # found the CMAKE_PREFIX_PATH e.g. Qt, Clang
  configure_file(InstallDependentSharedObjects.cmake.in InstallDependentSharedObjects.cmake @ONLY)
  install(SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/InstallDependentSharedObjects.cmake)

  if (MSVC)
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP ON)
    include(InstallRequiredsystemLibraries)

    install(PROGRAMS ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS}
      DESTINATION ${IDE_APP_PATH}
    )
  endif()
endif()
