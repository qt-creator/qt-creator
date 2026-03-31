function (env_with_default envName varToSet default)
  if(DEFINED ENV{${envName}})
    set(${varToSet} $ENV{${envName}} PARENT_SCOPE)
  else()
    set(${varToSet} ${default} PARENT_SCOPE)
  endif()
endfunction()

function(setup_dependencies_component)
  find_package(Python3 COMPONENTS Interpreter)
  if (NOT Python3_Interpreter_FOUND)
    message("No python interpreter found, skipping \"Dependencies\" install component.")
  else()
    get_target_property(_qmake_binary Qt::qmake IMPORTED_LOCATION)
    set(_llvm_arg)
    if (LLVM_INSTALL_PREFIX)
      set(_llvm_arg "--llvm \"${LLVM_INSTALL_PREFIX}\"")
    endif()
    set(_elfutils_arg)
    if (ELFUTILS_INCLUDE_DIR)
      get_filename_component(_elfutils_path ${ELFUTILS_INCLUDE_DIR} DIRECTORY)
      set(_elfutils_arg "--elfutils \"${_elfutils_path}\"")
    endif()
    install(CODE "
        if (CMAKE_VERSION GREATER_EQUAL 3.19)
          set(QTC_COMMAND_ERROR_IS_FATAL COMMAND_ERROR_IS_FATAL ANY)
        endif()
        # DESTDIR is set for e.g. the cpack DEB generator, but is empty in other situations
        if(DEFINED ENV{DESTDIR})
          set(DESTDIR_WITH_SEP \"\$ENV{DESTDIR}/\")
        else()
          set(DESTDIR_WITH_SEP \"\")
        endif()
        set(_default_app_target \"\${DESTDIR_WITH_SEP}\${CMAKE_INSTALL_PREFIX}/${IDE_APP_PATH}/${IDE_APP_TARGET}${CMAKE_EXECUTABLE_SUFFIX}\")
        set(_ide_app_target \"\${_default_app_target}\")
        if (NOT EXISTS \"\${_ide_app_target}\")
          # The component CPack generators (WIX, NSIS64, IFW) install every component with their own CMAKE_INSTALL_PREFIX
          # directory and since deploy.py needs the path to IDE_APP_TARGET the line below is needeed
          string(REPLACE \"Dependencies\" \"${CMAKE_INSTALL_DEFAULT_COMPONENT_NAME}\" _ide_app_target \"\${_ide_app_target}\")
        endif()
        if (NOT EXISTS \"\${_ide_app_target}\")
          # something went wrong, reset to default and hope for the best
          set(_ide_app_target \"\${_default_app_target}\")
        endif()
        execute_process(COMMAND
          \"${Python3_EXECUTABLE}\"
          \"-u\"
          \"${CMAKE_CURRENT_LIST_DIR}/scripts/deploy.py\"
          ${_llvm_arg}
          ${_elfutils_arg}
          \"\${_ide_app_target}\"
          \"${_qmake_binary}\"
          COMMAND_ECHO STDOUT
          ${QTC_COMMAND_ERROR_IS_FATAL}
          )
      "
      COMPONENT Dependencies
      EXCLUDE_FROM_ALL
    )
  endif()
endfunction()

function(create_standalone_install_component component_name)
  set(opt_args)
  set(single_args
    TARGET
  )
  set(multi_args
    PLUGINS
    IMPORTS
    RESOURCES
  )

  cmake_parse_arguments(_arg "${opt_args}" "${single_args}" "${multi_args}" ${ARGN})

  if (APPLE)
    set(library_dest "Frameworks")
    set(plugin_dest "PlugIns/qtcreator")
    set(libexec_dest "Resources/libexec")
    set(resource_dest "Resources")
    set(qt_library_dest "Frameworks")
    set(qt_plugin_dest "PlugIns")
    set(qt_import_dest "Imports/qtquick2")
    set(qt_conf [=[[Paths]
Prefix=../../
Binaries=MacOS
Libraries=Frameworks
Plugins=PlugIns
Qml2Imports=Imports/qtquick2
]=]
    )
  elseif (WIN32)
    set(library_dest "bin")
    set(plugin_dest "bin")
    set(libexec_dest "bin")
    set(resource_dest "share/qtcreator")
    set(qt_library_dest "bin")
    set(qt_plugin_dest "bin/plugins")
    set(qt_import_dest "bin/qml")
    set(soname_file "")
    set(qt_conf [=[[Paths]
Prefix=.
Binaries=.
Libraries=.
Plugins=plugins
Qml2Imports=qml
  ]=]
    )
  else()
    set(library_dest "${CMAKE_INSTALL_LIBDIR}/qtcreator")
    set(plugin_dest "${CMAKE_INSTALL_LIBDIR}/qtcreator/plugins")
    set(libexec_dest "${CMAKE_INSTALL_LIBEXECDIR}/qtcreator")
    set(resource_dest "${CMAKE_INSTALL_DATAROOTDIR}/qtcreator")
    set(qt_library_dest "lib/Qt/lib")
    set(qt_plugin_dest "lib/Qt/plugins")
    set(qt_import_dest "lib/Qt/qml")
    set(soname_file "\$<TARGET_SONAME_FILE:\${lib}>")
    set(qt_conf [=[[Paths]
Prefix=../../lib/Qt
Binaries=bin
Libraries=lib
Plugins=plugins
Qml2Imports=qml
  ]=]
    )
  endif()

  install(TARGETS ${_arg_TARGET}
    DESTINATION ${libexec_dest}
    COMPONENT ${component_name}
    EXCLUDE_FROM_ALL
  )

  # function for collecting all dependencies
  function(all_dependencies out_var targets)
    set(visited "")
    set(to_iterate ${targets})
    list(POP_BACK to_iterate next_target)
    while(next_target)
      if (TARGET ${next_target})
        get_target_property(_alias_target ${next_target} ALIASED_TARGET)
        if (_alias_target)
          set(next_target ${_alias_target})
        endif()
      endif()
      list(FIND visited ${next_target} found)
      if (NOT found STREQUAL "-1")
        list(POP_BACK to_iterate next_target)
        continue()
      endif()
      list(APPEND visited ${next_target})
      if (NOT TARGET ${next_target})
        list(POP_BACK to_iterate next_target)
        continue()
      endif()
      list(APPEND result "${next_target}")
      get_target_property(_linked_libs ${next_target} LINK_LIBRARIES)
      if (_linked_libs)
        list(APPEND to_iterate ${_linked_libs})
      endif()
      get_target_property(_interface_linked_libs ${next_target} INTERFACE_LINK_LIBRARIES)
      if (_interface_linked_libs)
        list(APPEND to_iterate ${_interface_linked_libs})
      endif()
      get_target_property(_imported_linked_libs ${next_target} IMPORTED_LINK_DEPENDENT_LIBRARIES)
      if (_imported_linked_libs)
        list(APPEND to_iterate ${_imported_linked_libs})
      endif()
      list(POP_BACK to_iterate next_target)
    endwhile()
    list(REMOVE_DUPLICATES result)
    set(${out_var} "${result}" PARENT_SCOPE)
  endfunction()

  # install libraries and frameworks that it depends on
  all_dependencies(linked_libs ${_arg_TARGET})
  ## plus the Qt Quick Controls plugin dependencies
  ## and wayland and what else is necessary
  set(additional_targets)
  if (_arg_IMPORTS)
    list(APPEND additional_targets
      QuickControls2
      QuickControls2Impl
      QuickTemplates2
      QuickControls2MacOSStyleImpl
      QuickControls2Fusion
      QuickControls2FusionStyleImpl
      QuickControls2Basic
      QuickControls2BasicStyleImpl
      QuickLayouts
      QuickShapes
      QuickShapesPrivate
      QuickEffects
      QmlMeta
      QmlModels
      OpenGL
      QmlWorkerScript
    )
  endif()
  if (UNIX AND NOT APPLE)
    list(APPEND additional_targets
      WaylandClient
      XcbQpaPrivate
      Svg # needed for Wayland
    )
  endif()
  foreach (fw IN LISTS additional_targets)
    find_package(Qt6 COMPONENTS ${fw} QUIET)
    if (TARGET Qt6::${fw})
      list(APPEND linked_libs Qt6::${fw})
    endif()
  endforeach()

  # CMake targets
  foreach (lib ${linked_libs})
    if (WIN32)
      set(soname_file "")
    else()
      set(soname_file "\$<TARGET_SONAME_FILE:${lib}>")
    endif()

    if (${lib} IN_LIST __QTC_PLUGINS)
      set(destination ${plugin_dest})
    elseif(${lib} IN_LIST __QTC_LIBRARIES)
      set(destination ${library_dest})
    else()
      set(destination ${qt_library_dest})
    endif()
    get_target_property(lib_type ${lib} TYPE)
    if (lib_type STREQUAL "SHARED_LIBRARY")
      get_target_property(is_framework ${lib} FRAMEWORK)
      if (NOT is_framework)
        install(FILES
          $<TARGET_FILE:${lib}>
          ${soname_file}
          DESTINATION ${destination}
          COMPONENT ${component_name}
          EXCLUDE_FROM_ALL
        )
      else()
        get_target_property(location ${lib} LOCATION)
        string(REGEX REPLACE "(.*[.]framework)/.*" "\\1" location ${location})
        install(DIRECTORY ${location}
          DESTINATION ${destination}
          COMPONENT ${component_name}
          EXCLUDE_FROM_ALL
          PATTERN "Headers" EXCLUDE
        )
      endif()
    endif()
  endforeach()

  # Qt plugins
  foreach (plugin IN LISTS _arg_PLUGINS)
    set(plugin_path ${QT6_INSTALL_PREFIX}/${QT6_INSTALL_PLUGINS}/${plugin})
    if (EXISTS "${plugin_path}")
      install(DIRECTORY "${plugin_path}"
        DESTINATION ${qt_plugin_dest}
        COMPONENT ${component_name}
        EXCLUDE_FROM_ALL
        PATTERN "*.cpp.o" EXCLUDE
      )
    endif()
  endforeach()

  # Qt Quick imports
  foreach (import IN LISTS _arg_IMPORTS)
    set(import_path ${QT6_INSTALL_PREFIX}/${QT6_INSTALL_QML}/${import})
    if (EXISTS "${import_path}")
      install(DIRECTORY "${import_path}"
        DESTINATION ${qt_import_dest}
        COMPONENT ${component_name}
        EXCLUDE_FROM_ALL
        PATTERN "*.cpp.o" EXCLUDE
      )
    endif()
  endforeach()

  # possibly ICU for Qt
  file(GLOB iculibs ${QT6_INSTALL_PREFIX}/${QT6_INSTALL_LIBS}/libicu*.so*)
  install(FILES ${iculibs}
    DESTINATION ${qt_library_dest}
    COMPONENT ${component_name}
    EXCLUDE_FROM_ALL
  )

  # Resources from Qt Creator
  foreach (res IN LISTS _arg_RESOURCES)
    qtc_output_binary_dir(_output_binary_dir)
    install(DIRECTORY ${_output_binary_dir}/${IDE_DATA_PATH}/${res}
      DESTINATION ${resource_dest}
      COMPONENT ${component_name}
      EXCLUDE_FROM_ALL
    )
  endforeach()

  # Write qt.conf for libexec
  install(CODE "file(WRITE \"\${CMAKE_INSTALL_PREFIX}/${libexec_dest}/qt.conf\"
               \"${qt_conf}\")"
    COMPONENT ${component_name}
    EXCLUDE_FROM_ALL
  )
endfunction()

function(configure_qml_designer Qt6_VERSION)
    set(QMLDESIGNER_QT6_REQUIRED_VERSION 6.7.3)
    set(QMLDESIGNER_GCC_REQUIRED_VERSION 10.0)
    set(QMLDESIGNER_CLANG_REQUIRED_VERSION 13.0)
    set(QMLDESIGNER_APPLECLANG_REQUIRED_VERSION 15.0)
    set(QMLDESIGNER_MSVC_REQUIRED_VERSION 19.30) # means MSVC 2022

    set(IS_SUPPORTED_PROJECTSTORAGE_QT OFF)
    set(IS_SUPPORTED_PROJECTSTORAGE_QT "${IS_SUPPORTED_PROJECTSTORAGE_QT}" PARENT_SCOPE)

    if (Qt6_VERSION VERSION_GREATER_EQUAL ${QMLDESIGNER_QT6_REQUIRED_VERSION})
        if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND
            CMAKE_CXX_COMPILER_VERSION VERSION_LESS ${QMLDESIGNER_GCC_REQUIRED_VERSION})
            set(QTC_WITH_QMLDESIGNER_DEFAULT OFF)
            string(APPEND QMLDESIGNER_FEATURE_DESC " and at least GCC ${QMLDESIGNER_GCC_REQUIRED_VERSION}")
        elseif (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang" AND
                CMAKE_CXX_COMPILER_VERSION VERSION_LESS ${QMLDESIGNER_APPLECLANG_REQUIRED_VERSION})
            set(QTC_WITH_QMLDESIGNER_DEFAULT OFF)
            string(APPEND QMLDESIGNER_FEATURE_DESC " and at least AppleClang ${QMLDESIGNER_APPLECLANG_REQUIRED_VERSION}")
        elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND
                CMAKE_CXX_COMPILER_VERSION VERSION_LESS ${QMLDESIGNER_CLANG_REQUIRED_VERSION})
            set(QTC_WITH_QMLDESIGNER_DEFAULT OFF)
            string(APPEND QMLDESIGNER_FEATURE_DESC " and at least Clang ${QMLDESIGNER_CLANG_REQUIRED_VERSION}")
        elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" AND
                CMAKE_CXX_COMPILER_VERSION VERSION_LESS ${QMLDESIGNER_MSVC_REQUIRED_VERSION})
            set(QTC_WITH_QMLDESIGNER_DEFAULT OFF)
            string(APPEND QMLDESIGNER_FEATURE_DESC " and at least MSVC 2022(ver. ${QMLDESIGNER_MSVC_REQUIRED_VERSION})")
        else()
            set(QTC_WITH_QMLDESIGNER_DEFAULT ON)
        endif()
        if (NOT QTC_WITH_QMLDESIGNER_DEFAULT)
            string(APPEND QMLDESIGNER_FEATURE_DESC ", found ${CMAKE_CXX_COMPILER_VERSION}")
        endif()
    else()
        set(QTC_WITH_QMLDESIGNER_DEFAULT OFF)
    endif()

    if(NOT TARGET Qt::Quick)
        set(QTC_WITH_QMLDESIGNER_DEFAULT OFF)
    endif()

    env_with_default("QTC_WITH_QMLDESIGNER" ENV_QTC_WITH_QMLDESIGNER ${QTC_WITH_QMLDESIGNER_DEFAULT})
    option(WITH_QMLDESIGNER "Build QmlDesigner" ${ENV_QTC_WITH_QMLDESIGNER})
    add_feature_info("WITH_QMLDESIGNER" ${WITH_QMLDESIGNER} "${QMLDESIGNER_FEATURE_DESC}")

    if(USE_PROJECTSTORAGE AND NOT IS_SUPPORTED_PROJECTSTORAGE_QT)
        if(BUILD_DESIGNSTUDIO)
            set(_level FATAL_ERROR)
        else()
            set(_level WARNING)
        endif()
        message(${_level}
            "USE_PROJECTSTORAGE is enabled, but current Qt ${Qt6_VERSION} is not supported by the project storage "
            "(required: ${PROJECTSTORAGE_QT_MIN_VERSION} - ${PROJECTSTORAGE_QT_MAX_VERSION})."
        )
    endif()

    # to enable define QML_DOM_MSVC2019_COMPAT if necessary, see QTBUG-127761
    if(NOT DEFINED QT_BUILT_WITH_MSVC2019)
        get_target_property(QT_QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)

        execute_process(
            COMMAND dumpbin /headers ${QT_QMAKE_EXECUTABLE} | findstr /c:"linker version"
            OUTPUT_VARIABLE DUMPBIN_OUTPUT
        )

        string(FIND "${DUMPBIN_OUTPUT}" "14.2" QT_BUILT_WITH_MSVC2019)

        if(QT_BUILT_WITH_MSVC2019 GREATER -1)
            set(QT_BUILT_WITH_MSVC2019 TRUE CACHE BOOL "Qt was built with MSVC 2019")
        else()
            set(QT_BUILT_WITH_MSVC2019 FALSE CACHE BOOL "Qt was not built with MSVC 2019")
        endif()
    endif()
endfunction()
