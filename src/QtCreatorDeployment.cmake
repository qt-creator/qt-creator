# Dependencies component
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
    set(qt_conf_dir ${IDE_APP_PATH})
  elseif(APPLE)
    set(qt5_plugin_dest_dir ${IDE_PLUGIN_PATH})
    set(qt5_qml_dest_dir ${IDE_DATA_PATH}/../Imports/qtquick2)
    set(qt_conf_dir ${IDE_DATA_PATH})
  else()
    set(qt5_plugin_dest_dir ${IDE_LIBRARY_BASE_PATH}/Qt/plugins)
    set(qt5_qml_dest_dir ${IDE_LIBRARY_BASE_PATH}/Qt/qml)
    set(qt_conf_dir ${IDE_APP_PATH})
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
      COMPONENT Dependencies EXCLUDE_FROM_ALL
      ${exclusion_mask}
    )
    list(APPEND qt5_plugin_directories "${qt5_plugin_dest_dir}/${plugin}")
  endforeach()

  install(
    DIRECTORY "${qt5_base_dir}/qml/"
    DESTINATION ${qt5_qml_dest_dir}
    COMPONENT Dependencies EXCLUDE_FROM_ALL
    PATTERN "qml/*"
    ${exclusion_mask}
  )

  install(CODE "
    get_filename_component(install_prefix \"\${CMAKE_INSTALL_PREFIX}\" ABSOLUTE)
    file(RELATIVE_PATH qt_conf_binaries
      \"\${install_prefix}/${qt_conf_dir}\"
      \"\${install_prefix}/${IDE_BIN_PATH}\"
    )
    if (NOT qt_conf_binaries)
      set(qt_conf_binaries .)
    endif()
    file(RELATIVE_PATH qt_conf_plugins
      \"\${install_prefix}/${qt_conf_dir}\"
      \"\${install_prefix}/${qt5_plugin_dest_dir}\"
    )
    file(RELATIVE_PATH qt_conf_qml
      \"\${install_prefix}/${qt_conf_dir}\"
      \"\${install_prefix}/${qt5_qml_dest_dir}\"
    )

    file(WRITE \"\${CMAKE_INSTALL_PREFIX}/${qt_conf_dir}/qt.conf\"
      \"[Paths]\n\"
      \"Binaries=\${qt_conf_binaries}\n\"
      \"Plugins=\${qt_conf_plugins}\n\"
      \"Qml2Imports=\${qt_conf_qml}\n\"
     )
    "
    COMPONENT Dependencies EXCLUDE_FROM_ALL
   )

  # Analyze the binaries and install missing dependencies if they are
  # found the CMAKE_PREFIX_PATH e.g. Qt, Clang
  install(CODE
    "
    if (MINGW AND CMAKE_CXX_COMPILER_ID MATCHES \"Clang\")
      set(CMAKE_GET_RUNTIME_DEPENDENCIES_TOOL objdump)
    endif()
    if (WIN32)
      set(pre_exclude_regexes PRE_EXCLUDE_REGEXES \"api-ms-.*|ext-ms-\.*\")
    endif()
    if (APPLE)
      set(pre_exclude_regexes PRE_EXCLUDE_REGEXES \"libiodbc\.*|libpq\.*\")
    endif()

    get_filename_component(install_prefix \"\${CMAKE_INSTALL_PREFIX}\" ABSOLUTE)

    # Get the dependencies of Qt's plugins
    foreach(plugin ${qt5_plugin_directories})
      file(GLOB plugin_files \"\${install_prefix}/\${plugin}/*${CMAKE_SHARED_LIBRARY_SUFFIX}\")
      list(APPEND qt5_plugin_files \"\${plugin_files}\")
    endforeach()

    # Get the qml module dependencies
    file(GLOB_RECURSE qml_plugin_files \"\${install_prefix}/\${qt5_qml_dest_dir}/*/*${CMAKE_SHARED_LIBRARY_SUFFIX}\")
    list(APPEND qt5_plugin_files \"\${qml_plugin_files}\")

    set(installed_EXECUTABLES_NOT_PREFIXED \"${__QTC_INSTALLED_EXECUTABLES}\")
    set(installed_LIBRARIES_NOT_PREFIXED \"${__QTC_INSTALLED_LIBRARIES}\")
    set(installed_MODULES_NOT_PREFIXED \"${__QTC_INSTALLED_PLUGINS}\")

    foreach(binary_type EXECUTABLES LIBRARIES MODULES)
      foreach(element IN LISTS installed_\${binary_type}_NOT_PREFIXED)
        if (EXISTS \"\${install_prefix}/\${element}\")
          list(APPEND installed_\${binary_type} \"\${install_prefix}/\${element}\")
        endif()
      endforeach()
    endforeach()

    # Install first the dependencies, and in second step the dependencies
    # from the installed dependencies e.g. libicu for libQt5Core on Linux.
    foreach(step 1 2)
      foreach(binary_type EXECUTABLES LIBRARIES MODULES)
        list(LENGTH installed_\${binary_type} list_size)
        if (NOT list_size EQUAL 0)
          set(\${binary_type}_TO_ANALYZE \${binary_type} \"\${installed_\${binary_type}}\")
        else()
          set(\${binary_type}_TO_ANALYZE \"\")
        endif()
      endforeach()

      file(GET_RUNTIME_DEPENDENCIES
        UNRESOLVED_DEPENDENCIES_VAR unresolved_deps
        \${EXECUTABLES_TO_ANALYZE}
        \${LIBRARIES_TO_ANALYZE}
        \${MODULES_TO_ANALYZE}
        DIRECTORIES
          \"\${install_prefix}/${IDE_PLUGIN_PATH}\"
          \"\${install_prefix}/${IDE_LIBEXEC_PATH}\"
          \"\${install_prefix}/${IDE_LIBRARY_PATH}\"
        \${pre_exclude_regexes}
      )

      # Clear for second step
      set(installed_EXECUTABLES \"\")
      set(installed_LIBRARIES \"\")
      set(installed_MODULES \"\${qt5_plugin_files}\")

      list(REMOVE_DUPLICATES unresolved_deps)

      if (WIN32)
        # Needed by QmlDesigner, QmlProfiler, but they are not referenced directly.
        list(APPEND unresolved_deps libEGL.dll libGLESv2.dll)
      endif()

      file(TO_CMAKE_PATH \"${CMAKE_PREFIX_PATH}\" prefix_path)

      # Add parent link directory paths. Needed for e.g. MinGW choco libstdc++-6.dll
      foreach(path \"${CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES}\")
        get_filename_component(parent_path \"\${path}\" DIRECTORY)
        list(APPEND prefix_path \"\${parent_path}\")
      endforeach()

      foreach(so IN LISTS unresolved_deps)
        string(REPLACE \"@rpath/\" \"\" so \"\${so}\")
        get_filename_component(so_dir \"\${so}\" DIRECTORY)
        message(STATUS \"Unresolved dependency: \${so}\")
        foreach(p IN LISTS prefix_path)
          message(STATUS \"Trying: \${p}/\${so}\")
          if (EXISTS \"\${p}/\${so}\")
            file(INSTALL \"\${p}/\${so}\"
              DESTINATION \"\${install_prefix}/${IDE_APP_PATH}/\${so_dir}\"
              FOLLOW_SYMLINK_CHAIN)
            list(APPEND installed_LIBRARIES \"\${install_prefix}/${IDE_APP_PATH}/\${so}\")
            break()
          endif()
          message(STATUS \"Trying: \${p}/bin/\${so}\")
          if (EXISTS \"\${p}/bin/\${so}\")
            file(INSTALL \"\${p}/bin/\${so}\"
              DESTINATION \"\${install_prefix}/${IDE_BIN_PATH}/\${so_dir}\"
              FOLLOW_SYMLINK_CHAIN)
            list(APPEND installed_LIBRARIES \"\${install_prefix}/${IDE_BIN_PATH}/\${so}\")
            break()
          endif()
          message(STATUS \"Trying: \${p}/lib/\${so}\")
          if (EXISTS \"\${p}/lib/\${so}\")
            file(INSTALL \"\${p}/lib/\${so}\"
              DESTINATION \"\${install_prefix}/${IDE_LIBRARY_PATH}/\${so_dir}\"
              FOLLOW_SYMLINK_CHAIN)
            list(APPEND installed_LIBRARIES \"\${install_prefix}/${IDE_LIBRARY_PATH}/\${so}\")
            break()
          endif()
        endforeach()
      endforeach()
    endforeach()
    "
    COMPONENT Dependencies EXCLUDE_FROM_ALL
  )

  if (MSVC)
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP ON)
    include(InstallRequiredsystemLibraries)

    install(PROGRAMS ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS}
      DESTINATION ${IDE_APP_PATH}
      COMPONENT Dependencies EXCLUDE_FROM_ALL
    )
  endif()

endif()
