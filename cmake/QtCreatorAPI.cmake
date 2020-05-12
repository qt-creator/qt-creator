if(QT_CREATOR_API_DEFINED)
  return()
endif()
set(QT_CREATOR_API_DEFINED TRUE)

if (CMAKE_VERSION VERSION_LESS 3.16)
  set(BUILD_WITH_PCH OFF)
endif()

include(FeatureSummary)

#
# Default Qt compilation defines
#

list(APPEND DEFAULT_DEFINES
  QT_CREATOR
  QT_NO_JAVA_STYLE_ITERATORS
  QT_NO_CAST_TO_ASCII QT_RESTRICTED_CAST_FROM_ASCII
  QT_DISABLE_DEPRECATED_BEFORE=0x050900
  QT_USE_FAST_OPERATOR_PLUS
  QT_USE_FAST_CONCATENATION
)

if (WIN32)
  list(APPEND DEFAULT_DEFINES UNICODE _UNICODE _CRT_SECURE_NO_WARNINGS)

  if (NOT BUILD_WITH_PCH)
    # Windows 8 0x0602
    list(APPEND DEFAULT_DEFINES
      WINVER=0x0602 _WIN32_WINNT=0x0602
      WIN32_LEAN_AND_MEAN)
  endif()
endif()

#
# Setup path handling
#

if (APPLE)
  set(_IDE_APP_PATH ".")
  set(_IDE_APP_TARGET "${IDE_DISPLAY_NAME}")

  set(_IDE_OUTPUT_PATH "${_IDE_APP_PATH}/${_IDE_APP_TARGET}.app/Contents")

  set(_IDE_PLUGIN_PATH "${_IDE_OUTPUT_PATH}/PlugIns")
  set(_IDE_LIBRARY_BASE_PATH "Frameworks")
  set(_IDE_LIBRARY_PATH "${_IDE_OUTPUT_PATH}/Frameworks")
  set(_IDE_LIBEXEC_PATH "${_IDE_OUTPUT_PATH}/Resources/libexec")
  set(_IDE_DATA_PATH "${_IDE_OUTPUT_PATH}/Resources")
  set(_IDE_DOC_PATH "${_IDE_OUTPUT_PATH}/Resources/doc")
  set(_IDE_BIN_PATH "${_IDE_OUTPUT_PATH}/MacOS")

  set(QT_DEST_PLUGIN_PATH "${_IDE_PLUGIN_PATH}")
  set(QT_DEST_QML_PATH "${_IDE_DATA_PATH}/../Imports/qtquick2")
else ()
  set(_IDE_APP_PATH "bin")
  set(_IDE_APP_TARGET "${IDE_ID}")

  set(_IDE_LIBRARY_BASE_PATH "lib")
  set(_IDE_LIBRARY_PATH "lib/${IDE_ID}")
  set(_IDE_PLUGIN_PATH "lib/${IDE_ID}/plugins")
  if (WIN32)
    set(_IDE_LIBEXEC_PATH "bin")
    set(QT_DEST_PLUGIN_PATH "bin/plugins")
    set(QT_DEST_QML_PATH "bin/qml")
  else ()
    set(_IDE_LIBEXEC_PATH "libexec/${IDE_ID}")
    set(QT_DEST_PLUGIN_PATH  "lib/Qt/plugins")
    set(QT_DEST_QML_PATH "lib/Qt/qml")
  endif ()
  set(_IDE_DATA_PATH "share/${IDE_ID}")
  set(_IDE_DOC_PATH "share/doc/${IDE_ID}")
  set(_IDE_BIN_PATH "bin")
endif ()

set(IDE_APP_PATH "${_IDE_APP_PATH}")                    # The target path of the IDE application (relative to CMAKE_INSTALL_PREFIX).
set(IDE_APP_TARGET "${_IDE_APP_TARGET}")                # The IDE application name.
set(IDE_PLUGIN_PATH "${_IDE_PLUGIN_PATH}")              # The IDE plugin path (relative to CMAKE_INSTALL_PREFIX).
set(IDE_LIBRARY_BASE_PATH "${_IDE_LIBRARY_BASE_PATH}")  # The IDE library base path (relative to CMAKE_INSTALL_PREFIX).
set(IDE_LIBRARY_PATH "${_IDE_LIBRARY_PATH}")            # The IDE library path (relative to CMAKE_INSTALL_PREFIX).
set(IDE_LIBEXEC_PATH "${_IDE_LIBEXEC_PATH}")            # The IDE libexec path (relative to CMAKE_INSTALL_PREFIX).
set(IDE_DATA_PATH "${_IDE_DATA_PATH}")                  # The IDE data path (relative to CMAKE_INSTALL_PREFIX).
set(IDE_DOC_PATH "${_IDE_DOC_PATH}")                    # The IDE documentation path (relative to CMAKE_INSTALL_PREFIX).
set(IDE_BIN_PATH "${_IDE_BIN_PATH}")                    # The IDE bin path (relative to CMAKE_INSTALL_PREFIX).

file(RELATIVE_PATH RELATIVE_PLUGIN_PATH "/${IDE_BIN_PATH}" "/${IDE_PLUGIN_PATH}")
file(RELATIVE_PATH RELATIVE_LIBEXEC_PATH "/${IDE_BIN_PATH}" "/${IDE_LIBEXEC_PATH}")
file(RELATIVE_PATH RELATIVE_DATA_PATH "/${IDE_BIN_PATH}" "/${IDE_DATA_PATH}")
file(RELATIVE_PATH RELATIVE_DOC_PATH "/${IDE_BIN_PATH}" "/${IDE_DOC_PATH}")

list(APPEND DEFAULT_DEFINES
  RELATIVE_PLUGIN_PATH="${RELATIVE_PLUGIN_PATH}"
  RELATIVE_LIBEXEC_PATH="${RELATIVE_LIBEXEC_PATH}"
  RELATIVE_DATA_PATH="${RELATIVE_DATA_PATH}"
  RELATIVE_DOC_PATH="${RELATIVE_DOC_PATH}"
)

file(RELATIVE_PATH _PLUGIN_TO_LIB "/${IDE_PLUGIN_PATH}" "/${IDE_LIBRARY_PATH}")
file(RELATIVE_PATH _PLUGIN_TO_QT "/${IDE_PLUGIN_PATH}" "/${IDE_LIBRARY_BASE_PATH}/Qt/lib")
file(RELATIVE_PATH _LIB_TO_QT "/${IDE_LIBRARY_PATH}" "/${IDE_LIBRARY_BASE_PATH}/Qt/lib")

if (APPLE)
  set(_RPATH_BASE "@executable_path")
  set(_LIB_RPATH "@loader_path")
  set(_PLUGIN_RPATH "@loader_path;@loader_path/${_PLUGIN_TO_LIB}")
elseif (WIN32)
  set(_RPATH_BASE "")
  set(_LIB_RPATH "")
  set(_PLUGIN_RPATH "")
else()
  set(_RPATH_BASE "\$ORIGIN")
  set(_LIB_RPATH "\$ORIGIN;\$ORIGIN/${_LIB_TO_QT}")
  set(_PLUGIN_RPATH "\$ORIGIN;\$ORIGIN/${_PLUGIN_TO_LIB};\$ORIGIN/${_PLUGIN_TO_QT}")
endif ()

set(__QTC_PLUGINS "" CACHE INTERNAL "*** Internal ***")
set(__QTC_INSTALLED_PLUGINS "" CACHE INTERNAL "*** Internal ***")
set(__QTC_LIBRARIES "" CACHE INTERNAL "*** Internal ***")
set(__QTC_INSTALLED_LIBRARIES "" CACHE INTERNAL "*** Internal ***")
set(__QTC_EXECUTABLES "" CACHE INTERNAL "*** Internal ***")
set(__QTC_INSTALLED_EXECUTABLES "" CACHE INTERNAL "*** Internal ***")
set(__QTC_TESTS "" CACHE INTERNAL "*** Internal ***")

#
# Internal functions
#

function(append_extra_translations target_name)
  if(NOT ARGN)
    return()
  endif()

  if(TARGET "${target_name}")
    get_target_property(_input "${target_name}" QT_EXTRA_TRANSLATIONS)
    if (_input)
      set(_output "${_input}" "${ARGN}")
    else()
      set(_output "${ARGN}")
    endif()
    set_target_properties("${target_name}" PROPERTIES QT_EXTRA_TRANSLATIONS "${_output}")
  endif()
endfunction()

function(update_cached_list name value)
  set(_tmp_list "${${name}}")
  list(APPEND _tmp_list "${value}")
  set("${name}" "${_tmp_list}" CACHE INTERNAL "*** Internal ***")
endfunction()

function(compare_sources_with_existing_disk_files target_name sources)
  if(NOT WITH_DEBUG_CMAKE)
    return()
  endif()

  file(GLOB_RECURSE existing_files RELATIVE ${CMAKE_CURRENT_LIST_DIR} "*.cpp" "*.hpp" "*.c" "*.h" "*.ui" "*.qrc")
  foreach(file IN LISTS existing_files)
    if(NOT ${file} IN_LIST sources)
      if (NOT WITH_TESTS AND ${file} MATCHES "test")
        continue()
      endif()
      message(STATUS "${target_name} doesn't include ${file}")
    endif()
  endforeach()

  foreach(source IN LISTS "${sources}")
    if(NOT ${source} IN_LIST existing_files)
      if (NOT WITH_TESTS AND ${file} MATCHES "test")
        continue()
      endif()
      message(STATUS "${target_name} contains non existing ${source}")
    endif()
  endforeach()
endfunction(compare_sources_with_existing_disk_files)

function(separate_object_libraries libraries REGULAR_LIBS OBJECT_LIBS OBJECT_LIB_OBJECTS)
  if (CMAKE_VERSION VERSION_LESS 3.14)
    foreach(lib IN LISTS libraries)
      if (TARGET ${lib})
        get_target_property(lib_type ${lib} TYPE)
        if (lib_type STREQUAL "OBJECT_LIBRARY")
          list(APPEND object_libs ${lib})
          list(APPEND object_libs_objects $<TARGET_OBJECTS:${lib}>)
        else()
          list(APPEND regular_libs ${lib})
        endif()
      else()
        list(APPEND regular_libs ${lib})
      endif()
      set(${REGULAR_LIBS} ${regular_libs} PARENT_SCOPE)
      set(${OBJECT_LIBS} ${object_libs} PARENT_SCOPE)
      set(${OBJECT_LIB_OBJECTS} ${object_libs_objects} PARENT_SCOPE)
    endforeach()
  else()
    set(${REGULAR_LIBS} ${libraries} PARENT_SCOPE)
    unset(${OBJECT_LIBS} PARENT_SCOPE)
    unset(${OBJECT_LIB_OBJECTS} PARENT_SCOPE)
  endif()
endfunction(separate_object_libraries)

function(set_explicit_moc target_name file)
  unset(file_dependencies)
  if (file MATCHES "^.*plugin.h$")
    set(file_dependencies DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${target_name}.json")
  endif()
  set_property(SOURCE "${file}" PROPERTY SKIP_AUTOMOC ON)
  qt5_wrap_cpp(file_moc "${file}" ${file_dependencies})
  target_sources(${target_name} PRIVATE "${file_moc}")
endfunction()

function(set_public_headers target sources)
  foreach(source IN LISTS sources)
    if (source MATCHES "\.h$|\.hpp$")

      if (NOT IS_ABSOLUTE ${source})
        set(source "${CMAKE_CURRENT_SOURCE_DIR}/${source}")
      endif()

      get_filename_component(source_dir ${source} DIRECTORY)
      file(RELATIVE_PATH include_dir_relative_path ${PROJECT_SOURCE_DIR} ${source_dir})

      install(
        FILES ${source}
        DESTINATION "include/${include_dir_relative_path}"
        COMPONENT Devel EXCLUDE_FROM_ALL
      )
    endif()
  endforeach()
endfunction()

function(set_public_includes target includes)
  foreach(inc_dir IN LISTS includes)
    if (NOT IS_ABSOLUTE ${inc_dir})
      set(inc_dir "${CMAKE_CURRENT_SOURCE_DIR}/${inc_dir}")
    endif()
    file(RELATIVE_PATH include_dir_relative_path ${PROJECT_SOURCE_DIR} ${inc_dir})
    target_include_directories(${target} PUBLIC
      $<BUILD_INTERFACE:${inc_dir}>
      $<INSTALL_INTERFACE:include/${include_dir_relative_path}>
    )
  endforeach()
endfunction()

function(finalize_test_setup test_name)
  # Never translate tests:
  set_tests_properties(${name}
    PROPERTIES
      QT_SKIP_TRANSLATION ON
      TIMEOUT 5
  )

  if (WIN32)
    list(APPEND env_path $ENV{PATH})
    list(APPEND env_path ${CMAKE_BINARY_DIR}/${IDE_PLUGIN_PATH})
    list(APPEND env_path ${CMAKE_BINARY_DIR}/${IDE_BIN_PATH})
    list(APPEND env_path $<TARGET_FILE_DIR:Qt5::Test>)
    if (TARGET libclang)
        list(APPEND env_path $<TARGET_FILE_DIR:libclang>)
    endif()

    if (TARGET elfutils::elf)
        list(APPEND env_path $<TARGET_FILE_DIR:elfutils::elf>)
    endif()

    string(REPLACE "/" "\\" env_path "${env_path}")
    string(REPLACE ";" "\\;" env_path "${env_path}")

    set_tests_properties(${test_name} PROPERTIES ENVIRONMENT "PATH=${env_path}")
  endif()
endfunction()

function(add_qtc_depends target_name)
  cmake_parse_arguments(_arg "" "" "PRIVATE;PUBLIC" ${ARGN})
  if (${_arg_UNPARSED_ARGUMENTS})
    message(FATAL_ERROR "add_qtc_depends had unparsed arguments")
  endif()

  separate_object_libraries("${_arg_PRIVATE}"
    depends object_lib_depends object_lib_depends_objects)
  separate_object_libraries("${_arg_PUBLIC}"
    public_depends object_public_depends object_public_depends_objects)

  target_sources(${target_name} PRIVATE ${object_lib_depends_objects} ${object_public_depends_objects})

  get_target_property(target_type ${target_name} TYPE)
  if (NOT target_type STREQUAL "OBJECT_LIBRARY")
    target_link_libraries(${target_name} PRIVATE ${depends} PUBLIC ${public_depends})
  else()
    list(APPEND object_lib_depends ${depends})
    list(APPEND object_public_depends ${public_depends})
  endif()

  foreach(obj_lib IN LISTS object_lib_depends)
    target_compile_definitions(${target_name} PRIVATE $<TARGET_PROPERTY:${obj_lib},INTERFACE_COMPILE_DEFINITIONS>)
    target_include_directories(${target_name} PRIVATE $<TARGET_PROPERTY:${obj_lib},INTERFACE_INCLUDE_DIRECTORIES>)
  endforeach()
  foreach(obj_lib IN LISTS object_public_depends)
    target_compile_definitions(${target_name} PUBLIC $<TARGET_PROPERTY:${obj_lib},INTERFACE_COMPILE_DEFINITIONS>)
    target_include_directories(${target_name} PUBLIC $<TARGET_PROPERTY:${obj_lib},INTERFACE_INCLUDE_DIRECTORIES>)
  endforeach()
endfunction()

function(find_dependent_plugins varName)
  set(_RESULT ${ARGN})

  foreach(i ${ARGN})
    get_property(_dep TARGET "${i}" PROPERTY _arg_DEPENDS)
    if (_dep)
      find_dependent_plugins(_REC ${_dep})
      list(APPEND _RESULT ${_REC})
    endif()
  endforeach()

  if (_RESULT)
    list(REMOVE_DUPLICATES _RESULT)
    list(SORT _RESULT)
  endif()

  set("${varName}" ${_RESULT} PARENT_SCOPE)
endfunction()

function(qtc_plugin_enabled varName name)
  if (NOT (name IN_LIST __QTC_PLUGINS))
    message(FATAL_ERROR "extend_qtc_plugin: Unknown plugin target \"${name}\"")
  endif()
  if (TARGET ${name})
    set(${varName} ON PARENT_SCOPE)
  else()
    set(${varName} OFF PARENT_SCOPE)
  endif()
endfunction()

function(qtc_library_enabled varName name)
  if (NOT (name IN_LIST __QTC_LIBRARIES))
    message(FATAL_ERROR "extend_qtc_library: Unknown library target \"${name}\"")
  endif()
  if (TARGET ${name})
    set(${varName} ON PARENT_SCOPE)
  else()
    set(${varName} OFF PARENT_SCOPE)
  endif()
endfunction()

function(enable_pch target)
  if (BUILD_WITH_PCH)
    # Skip PCH for targets that do not use the expected visibility settings:
    get_target_property(visibility_property "${target}" CXX_VISIBILITY_PRESET)
    get_target_property(inlines_property "${target}" VISIBILITY_INLINES_HIDDEN)

    if (NOT visibility_property STREQUAL "hidden" OR NOT inlines_property)
      return()
    endif()

    # Skip PCH for targets that do not have QT_NO_CAST_TO_ASCII
    get_target_property(target_defines "${target}" COMPILE_DEFINITIONS)
    if (NOT "QT_NO_CAST_TO_ASCII" IN_LIST target_defines)
      return()
    endif()

    get_target_property(target_type ${target} TYPE)
    if (NOT ${target_type} STREQUAL "OBJECT_LIBRARY")
      function(_recursively_collect_dependencies input_target)
        get_target_property(input_type ${input_target} TYPE)
        if (${input_type} STREQUAL "INTERFACE_LIBRARY")
          set(prefix "INTERFACE_")
        endif()
        get_target_property(link_libraries ${input_target} ${prefix}LINK_LIBRARIES)
        foreach(library IN LISTS link_libraries)
          if(TARGET ${library} AND NOT ${library} IN_LIST dependencies)
            list(APPEND dependencies ${library})
            _recursively_collect_dependencies(${library})
          endif()
        endforeach()
        set(dependencies ${dependencies} PARENT_SCOPE)
      endfunction()
      _recursively_collect_dependencies(${target})

      function(_add_pch_target pch_target pch_file pch_dependency)
        if (EXISTS ${pch_file})
          add_library(${pch_target} STATIC
            ${CMAKE_CURRENT_BINARY_DIR}/empty_pch.cpp
            ${CMAKE_CURRENT_BINARY_DIR}/empty_pch.c)
          target_compile_definitions(${pch_target} PRIVATE ${DEFAULT_DEFINES})
          set_target_properties(${pch_target} PROPERTIES
            PRECOMPILE_HEADERS ${pch_file}
            CXX_VISIBILITY_PRESET hidden
            VISIBILITY_INLINES_HIDDEN ON)
          target_link_libraries(${pch_target} PRIVATE ${pch_dependency})
        endif()
      endfunction()

      if (NOT TARGET QtCreatorPchGui AND NOT TARGET QtCreatorPchConsole)
        file(GENERATE
          OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/empty_pch.c
          CONTENT "/*empty file*/")
        file(GENERATE
          OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/empty_pch.cpp
          CONTENT "/*empty file*/")
        _add_pch_target(QtCreatorPchGui
          "${PROJECT_SOURCE_DIR}/src/shared/qtcreator_gui_pch.h" Qt5::Widgets)
        _add_pch_target(QtCreatorPchConsole
          "${PROJECT_SOURCE_DIR}/src/shared/qtcreator_pch.h" Qt5::Core)
      endif()

      unset(PCH_TARGET)
      if ("Qt5::Widgets" IN_LIST dependencies)
        set(PCH_TARGET QtCreatorPchGui)
      elseif ("Qt5::Core" IN_LIST dependencies)
        set(PCH_TARGET QtCreatorPchConsole)
      endif()

      if (TARGET "${PCH_TARGET}")
        set_target_properties(${target} PROPERTIES
          PRECOMPILE_HEADERS_REUSE_FROM ${PCH_TARGET})
      endif()
    endif()
  endif()
endfunction()

function(qtc_output_binary_dir varName)
  if (QTC_MERGE_BINARY_DIR)
    set(${varName} ${QtCreator_BINARY_DIR} PARENT_SCOPE)
  else()
    set(${varName} ${PROJECT_BINARY_DIR} PARENT_SCOPE)
  endif()
endfunction()

function(condition_info varName condition)
  if (NOT ${condition})
    set(${varName} "" PARENT_SCOPE)
  else()
    string(REPLACE ";" " " _contents "${${condition}}")
    set(${varName} "with CONDITION ${_contents}" PARENT_SCOPE)
  endif()
endfunction()

#
# Public API functions
#

function(add_qtc_library name)
  cmake_parse_arguments(_arg "STATIC;OBJECT;SKIP_TRANSLATION;BUILD_BY_DEFAULT;ALLOW_ASCII_CASTS;UNVERSIONED"
    "DESTINATION;COMPONENT"
    "DEFINES;DEPENDS;EXTRA_TRANSLATIONS;INCLUDES;PUBLIC_DEFINES;PUBLIC_DEPENDS;PUBLIC_INCLUDES;SOURCES;EXPLICIT_MOC;SKIP_AUTOMOC;PROPERTIES" ${ARGN}
  )

  set(default_defines_copy ${DEFAULT_DEFINES})
  if (_arg_ALLOW_ASCII_CASTS)
    list(REMOVE_ITEM default_defines_copy QT_NO_CAST_TO_ASCII QT_RESTRICTED_CAST_FROM_ASCII)
  endif()

  if (${_arg_UNPARSED_ARGUMENTS})
    message(FATAL_ERROR "add_qtc_library had unparsed arguments")
  endif()

  update_cached_list(__QTC_LIBRARIES "${name}")

  # special libraries can be turned off
  if (_arg_BUILD_BY_DEFAULT)
    string(TOUPPER "BUILD_LIBRARY_${name}" _build_library_var)
    set(_build_library_default "ON")
    if (DEFINED ENV{QTC_${_build_library_var}})
      set(_build_library_default "$ENV{QTC_${_build_library_var}}")
    endif()
    set(${_build_library_var} "${_build_library_default}" CACHE BOOL "Build library ${name}.")

    if (NOT ${_build_library_var})
      return()
    endif()
  endif()

  compare_sources_with_existing_disk_files(${name} "${_arg_SOURCES}")

  set(library_type SHARED)
  if (_arg_STATIC)
    set(library_type STATIC)
  endif()
  if (_arg_OBJECT)
    set(library_type OBJECT)
  endif()

  set(_exclude_from_all EXCLUDE_FROM_ALL)
  if (_arg_BUILD_BY_DEFAULT)
    unset(_exclude_from_all)
  endif()

  # Do not just build libraries...
  add_library(${name} ${library_type} ${_exclude_from_all} ${_arg_SOURCES})
  add_library(${IDE_CASED_ID}::${name} ALIAS ${name})
  set_public_headers(${name} "${_arg_SOURCES}")

  if (${name} MATCHES "^[^0-9-]+$")
    string(TOUPPER "${name}_LIBRARY" EXPORT_SYMBOL)
  endif()

  if (WITH_TESTS)
    set(TEST_DEFINES WITH_TESTS SRCDIR="${CMAKE_CURRENT_SOURCE_DIR}")
  endif()

  file(RELATIVE_PATH include_dir_relative_path ${PROJECT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

  target_include_directories(${name}
    PRIVATE
      ${_arg_INCLUDES}
      "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    PUBLIC
      "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>"
      "$<INSTALL_INTERFACE:include/${include_dir_relative_path}/..>"
  )
  set_public_includes(${name} "${_arg_PUBLIC_INCLUDES}")

  target_compile_definitions(${name}
    PRIVATE ${EXPORT_SYMBOL} ${default_defines_copy} ${_arg_DEFINES} ${TEST_DEFINES}
    PUBLIC ${_arg_PUBLIC_DEFINES}
  )

  add_qtc_depends(${name}
    PRIVATE ${_arg_DEPENDS} ${IMPLICIT_DEPENDS}
    PUBLIC ${_arg_PUBLIC_DEPENDS}
  )

  foreach(file IN LISTS _arg_EXPLICIT_MOC)
    set_explicit_moc(${name} "${file}")
  endforeach()

  foreach(file IN LISTS _arg_SKIP_AUTOMOC)
    set_property(SOURCE ${file} PROPERTY SKIP_AUTOMOC ON)
  endforeach()

  set(skip_translation OFF)
  if (_arg_SKIP_TRANSLATION)
    set(skip_translation ON)
  endif()

  set(_DESTINATION "${IDE_BIN_PATH}")
  if (_arg_DESTINATION)
    set(_DESTINATION "${_arg_DESTINATION}")
  endif()

  qtc_output_binary_dir(_output_binary_dir)
  set_target_properties(${name} PROPERTIES
    SOURCES_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
    VERSION "${IDE_VERSION}"
    SOVERSION "${PROJECT_VERSION_MAJOR}"
    CXX_EXTENSIONS OFF
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
    BUILD_RPATH "${_LIB_RPATH}"
    INSTALL_RPATH "${_LIB_RPATH}"
    RUNTIME_OUTPUT_DIRECTORY "${_output_binary_dir}/${_DESTINATION}"
    LIBRARY_OUTPUT_DIRECTORY "${_output_binary_dir}/${IDE_LIBRARY_PATH}"
    ARCHIVE_OUTPUT_DIRECTORY "${_output_binary_dir}/${IDE_LIBRARY_PATH}"
    ${_arg_PROPERTIES}
  )
  enable_pch(${name})

  if (WIN32 AND library_type STREQUAL "SHARED" AND NOT _arg_UNVERSIONED)
    # Match qmake naming scheme e.g. Library4.dll
    set_target_properties(${name} PROPERTIES
      SUFFIX "${PROJECT_VERSION_MAJOR}${CMAKE_SHARED_LIBRARY_SUFFIX}"
      PREFIX ""
    )
  endif()

  unset(NAMELINK_OPTION)
  if (library_type STREQUAL "SHARED")
    set(NAMELINK_OPTION NAMELINK_SKIP)
  endif()

  unset(COMPONENT_OPTION)
  if (_arg_COMPONENT)
    set(COMPONENT_OPTION "COMPONENT" "${_arg_COMPONENT}")
  endif()

  install(TARGETS ${name}
    EXPORT ${IDE_CASED_ID}
    RUNTIME
      DESTINATION "${_DESTINATION}"
      ${COMPONENT_OPTION}
      OPTIONAL
    LIBRARY
      DESTINATION "${IDE_LIBRARY_PATH}"
      ${NAMELINK_OPTION}
      ${COMPONENT_OPTION}
      OPTIONAL
    OBJECTS
      DESTINATION "${IDE_LIBRARY_PATH}"
      COMPONENT Devel EXCLUDE_FROM_ALL
    ARCHIVE
      DESTINATION "${IDE_LIBRARY_PATH}"
      COMPONENT Devel EXCLUDE_FROM_ALL
      OPTIONAL
  )

  if (library_type STREQUAL "SHARED")
    set(target_prefix ${CMAKE_SHARED_LIBRARY_PREFIX})
    if (WIN32)
      set(target_suffix ${PROJECT_VERSION_MAJOR}${CMAKE_SHARED_LIBRARY_SUFFIX})
      set(target_prefix "")
    elseif(APPLE)
      set(target_suffix .${PROJECT_VERSION_MAJOR}${CMAKE_SHARED_LIBRARY_SUFFIX})
    else()
      set(target_suffix ${CMAKE_SHARED_LIBRARY_SUFFIX}.${PROJECT_VERSION_MAJOR})
    endif()
    set(lib_dir "${IDE_LIBRARY_PATH}")
    if (WIN32)
      set(lib_dir "${_DESTINATION}")
    endif()
    update_cached_list(__QTC_INSTALLED_LIBRARIES
      "${lib_dir}/${target_prefix}${name}${target_suffix}")
  endif()

  if (NAMELINK_OPTION)
    install(TARGETS ${name}
      LIBRARY
        DESTINATION "${IDE_LIBRARY_PATH}"
        NAMELINK_ONLY
        COMPONENT Devel EXCLUDE_FROM_ALL
      OPTIONAL
    )
  endif()

  append_extra_translations("${name}" "${_arg_EXTRA_TRANSLATIONS}")
endfunction(add_qtc_library)

function(add_qtc_plugin target_name)
  cmake_parse_arguments(_arg
    "EXPERIMENTAL;SKIP_DEBUG_CMAKE_FILE_CHECK;SKIP_INSTALL;INTERNAL_ONLY;SKIP_TRANSLATION"
    "VERSION;COMPAT_VERSION;PLUGIN_JSON_IN;PLUGIN_PATH;PLUGIN_NAME;OUTPUT_NAME"
    "CONDITION;DEPENDS;EXTRA_TRANSLATIONS;PUBLIC_DEPENDS;DEFINES;PUBLIC_DEFINES;INCLUDES;PUBLIC_INCLUDES;PLUGIN_DEPENDS;PLUGIN_RECOMMENDS;SOURCES;EXPLICIT_MOC"
    ${ARGN}
  )

  if (${_arg_UNPARSED_ARGUMENTS})
    message(FATAL_ERROR "add_qtc_plugin had unparsed arguments")
  endif()

  update_cached_list(__QTC_PLUGINS "${target_name}")

  set(name ${target_name})
  if (_arg_PLUGIN_NAME)
    set(name ${_arg_PLUGIN_NAME})
  endif()

  condition_info(_extra_text _arg_CONDITION)
  if (NOT _arg_CONDITION)
    set(_arg_CONDITION ON)
  endif()

  string(TOUPPER "BUILD_PLUGIN_${target_name}" _build_plugin_var)
  set(_build_plugin_default "ON")
  if (DEFINED ENV{QTC_${_build_plugin_var}})
    set(_build_plugin_default "$ENV{QTC_${_build_plugin_var}}")
  endif()
  if (_arg_INTERNAL_ONLY)
    set(${_build_plugin_var} "${_build_plugin_default}")
  else()
    set(${_build_plugin_var} "${_build_plugin_default}" CACHE BOOL "Build plugin ${name}.")
  endif()

  if ((${_arg_CONDITION}) AND ${_build_plugin_var})
    set(_plugin_enabled ON)
  else()
    set(_plugin_enabled OFF)
  endif()

  if (NOT _arg_INTERNAL_ONLY)
    add_feature_info("Plugin ${name}" _plugin_enabled "${_extra_text}")
  endif()
  if (NOT _plugin_enabled)
    return()
  endif()

  ### Generate plugin.json file:
  if (NOT _arg_VERSION)
    set(_arg_VERSION ${IDE_VERSION})
  endif()
  if (NOT _arg_COMPAT_VERSION)
    set(_arg_COMPAT_VERSION ${_arg_VERSION})
  endif()

  if (NOT _arg_SKIP_DEBUG_CMAKE_FILE_CHECK)
    compare_sources_with_existing_disk_files(${target_name} "${_arg_SOURCES}")
  endif()

  # Generate dependency list:
  find_dependent_plugins(_DEP_PLUGINS ${_arg_PLUGIN_DEPENDS})

  set(_arg_DEPENDENCY_STRING "\"Dependencies\" : [\n")
  foreach(i IN LISTS _DEP_PLUGINS)
    if (i MATCHES "^${IDE_CASED_ID}::")
      set(_v ${IDE_VERSION})
      string(REPLACE "${IDE_CASED_ID}::" "" i ${i})
    else()
      get_property(_v TARGET "${i}" PROPERTY _arg_VERSION)
    endif()
    string(APPEND _arg_DEPENDENCY_STRING
      "        { \"Name\" : \"${i}\", \"Version\" : \"${_v}\" }"
    )
  endforeach(i)
  string(REPLACE "}        {" "},\n        {"
    _arg_DEPENDENCY_STRING "${_arg_DEPENDENCY_STRING}"
  )
  foreach(i IN LISTS ${_arg_RECOMMENDS})
    if (i MATCHES "^${IDE_CASED_ID}::")
      set(_v ${IDE_VERSION})
      string(REPLACE "${IDE_CASED_ID}::" "" i ${i})
    else()
      get_property(_v TARGET "${i}" PROPERTY _arg_VERSION)
    endif()
    string(APPEND _arg_DEPENDENCY_STRING
      "        { \"Name\" : \"${i}\", \"Version\" : \"${_v}\", \"Type\" : \"optional\" }"
    )
  endforeach(i)
  string(APPEND _arg_DEPENDENCY_STRING "\n    ]")
  if (_arg_EXPERIMENTAL)
    string(APPEND _arg_DEPENDENCY_STRING ",\n    \"Experimental\" : true")
  endif()

  set(IDE_PLUGIN_DEPENDENCY_STRING ${_arg_DEPENDENCY_STRING})

  ### Configure plugin.json file:
  if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${name}.json.in")
    file(READ "${name}.json.in" plugin_json_in)
    string(REPLACE "\\\"" "\"" plugin_json_in ${plugin_json_in})
    string(REPLACE "\\'" "'" plugin_json_in ${plugin_json_in})
    string(REPLACE "$$QTCREATOR_VERSION" "\${IDE_VERSION}" plugin_json_in ${plugin_json_in})
    string(REPLACE "$$QTCREATOR_COMPAT_VERSION" "\${IDE_VERSION_COMPAT}" plugin_json_in ${plugin_json_in})
    string(REPLACE "$$QTCREATOR_COPYRIGHT_YEAR" "\${IDE_COPYRIGHT_YEAR}" plugin_json_in ${plugin_json_in})
    string(REPLACE "$$QTC_PLUGIN_REVISION" "\${QTC_PLUGIN_REVISION}" plugin_json_in ${plugin_json_in})
    string(REPLACE "$$dependencyList" "\${IDE_PLUGIN_DEPENDENCY_STRING}" plugin_json_in ${plugin_json_in})
    if(_arg_PLUGIN_JSON_IN)
        #e.g. UPDATEINFO_EXPERIMENTAL_STR=true
        string(REGEX REPLACE "=.*$" "" json_key ${_arg_PLUGIN_JSON_IN})
        string(REGEX REPLACE "^.*=" "" json_value ${_arg_PLUGIN_JSON_IN})
        string(REPLACE "$$${json_key}" "${json_value}" plugin_json_in ${plugin_json_in})
    endif()
    string(CONFIGURE "${plugin_json_in}" plugin_json)
    file(GENERATE
      OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${name}.json"
      CONTENT "${plugin_json}")
  endif()

  add_library(${target_name} SHARED ${_arg_SOURCES})
  add_library(${IDE_CASED_ID}::${target_name} ALIAS ${target_name})
  set_public_headers(${target_name} "${_arg_SOURCES}")

  ### Generate EXPORT_SYMBOL
  string(TOUPPER "${name}_LIBRARY" EXPORT_SYMBOL)

  if (WITH_TESTS)
    set(TEST_DEFINES WITH_TESTS SRCDIR="${CMAKE_CURRENT_SOURCE_DIR}")
  endif()

  file(RELATIVE_PATH include_dir_relative_path ${PROJECT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

  target_include_directories(${target_name}
    PRIVATE
      ${_arg_INCLUDES}
      "${CMAKE_CURRENT_BINARY_DIR}"
      "${CMAKE_BINARY_DIR}/src"
      "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    PUBLIC
      "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>"
      "$<INSTALL_INTERFACE:include/${include_dir_relative_path}/..>"
  )
  set_public_includes(${target_name} "${_arg_PUBLIC_INCLUDES}")

  target_compile_definitions(${target_name}
    PRIVATE ${EXPORT_SYMBOL} ${DEFAULT_DEFINES} ${_arg_DEFINES} ${TEST_DEFINES}
    PUBLIC ${_arg_PUBLIC_DEFINES}
  )

  add_qtc_depends(${target_name}
    PRIVATE ${_arg_DEPENDS} ${_DEP_PLUGINS} ${IMPLICIT_DEPENDS}
    PUBLIC ${_arg_PUBLIC_DEPENDS}
  )

  set(plugin_dir "${IDE_PLUGIN_PATH}")
  if (_arg_PLUGIN_PATH)
    set(plugin_dir "${_arg_PLUGIN_PATH}")
  endif()

  set(skip_translation OFF)
  if (_arg_SKIP_TRANSLATION)
    set(skip_translation ON)
  endif()

  qtc_output_binary_dir(_output_binary_dir)
  set_target_properties(${target_name} PROPERTIES
    SOURCES_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
    CXX_EXTENSIONS OFF
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
    _arg_DEPENDS "${_arg_PLUGIN_DEPENDS}"
    _arg_VERSION "${_arg_VERSION}"
    BUILD_RPATH "${_PLUGIN_RPATH}"
    INSTALL_RPATH "${_PLUGIN_RPATH}"
    LIBRARY_OUTPUT_DIRECTORY "${_output_binary_dir}/${plugin_dir}"
    ARCHIVE_OUTPUT_DIRECTORY "${_output_binary_dir}/${plugin_dir}"
    RUNTIME_OUTPUT_DIRECTORY "${_output_binary_dir}/${plugin_dir}"
    OUTPUT_NAME "${name}"
    QT_SKIP_TRANSLATION "${skip_translation}"
    ${_arg_PROPERTIES}
  )

  if (WIN32)
    # Match qmake naming scheme e.g. Plugin4.dll
    set_target_properties(${target_name} PROPERTIES
      SUFFIX "${PROJECT_VERSION_MAJOR}${CMAKE_SHARED_LIBRARY_SUFFIX}"
      PREFIX ""
    )
  endif()
  append_extra_translations("${target_name}" "${_arg_EXTRA_TRANSLATIONS}")
  enable_pch(${target_name})

  foreach(file IN LISTS _arg_EXPLICIT_MOC)
    set_explicit_moc(${target_name} "${file}")
  endforeach()

  if (NOT _arg_SKIP_INSTALL)
    install(TARGETS ${target_name}
      EXPORT ${IDE_CASED_ID}
      RUNTIME DESTINATION "${plugin_dir}" OPTIONAL
      LIBRARY DESTINATION "${plugin_dir}" OPTIONAL
      ARCHIVE
        DESTINATION "${plugin_dir}"
        COMPONENT Devel EXCLUDE_FROM_ALL
        OPTIONAL
    )
    get_target_property(target_suffix ${target_name} SUFFIX)
    get_target_property(target_prefix ${target_name} PREFIX)
    if (target_suffix STREQUAL "target_suffix-NOTFOUND")
      set(target_suffix ${CMAKE_SHARED_LIBRARY_SUFFIX})
    endif()
    if (target_prefix STREQUAL "target_prefix-NOTFOUND")
      set(target_prefix ${CMAKE_SHARED_LIBRARY_PREFIX})
    endif()
    update_cached_list(__QTC_INSTALLED_PLUGINS
      "${plugin_dir}/${target_prefix}${target_name}${target_suffix}")
  endif()
endfunction()

function(extend_qtc_target target_name)
  cmake_parse_arguments(_arg
    ""
    "SOURCES_PREFIX;FEATURE_INFO"
    "CONDITION;DEPENDS;PUBLIC_DEPENDS;DEFINES;PUBLIC_DEFINES;INCLUDES;PUBLIC_INCLUDES;SOURCES;EXPLICIT_MOC"
    ${ARGN}
  )

  if (${_arg_UNPARSED_ARGUMENTS})
    message(FATAL_ERROR "extend_qtc_target had unparsed arguments")
  endif()

  condition_info(_extra_text _arg_CONDITION)
  if (NOT _arg_CONDITION)
    set(_arg_CONDITION ON)
  endif()
  if (${_arg_CONDITION})
    set(_feature_enabled ON)
  else()
    set(_feature_enabled OFF)
  endif()
  if (_arg_FEATURE_INFO)
    add_feature_info(${_arg_FEATURE_INFO} _feature_enabled "${_extra_text}")
  endif()
  if (NOT _feature_enabled)
    return()
  endif()

  add_qtc_depends(${target_name}
    PRIVATE ${_arg_DEPENDS}
    PUBLIC ${_arg_PUBLIC_DEPENDS}
  )
  target_compile_definitions(${target_name}
    PRIVATE ${_arg_DEFINES}
    PUBLIC ${_arg_PUBLIC_DEFINES}
  )
  target_include_directories(${target_name} PRIVATE ${_arg_INCLUDES})

  set_public_includes(${target_name} "${_arg_PUBLIC_INCLUDES}")

  if (_arg_SOURCES_PREFIX)
    foreach(source IN LISTS _arg_SOURCES)
      list(APPEND prefixed_sources "${_arg_SOURCES_PREFIX}/${source}")
    endforeach()

    if (NOT IS_ABSOLUTE ${_arg_SOURCES_PREFIX})
      set(_arg_SOURCES_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/${_arg_SOURCES_PREFIX}")
    endif()
    target_include_directories(${target_name} PRIVATE $<BUILD_INTERFACE:${_arg_SOURCES_PREFIX}>)

    set(_arg_SOURCES ${prefixed_sources})
  endif()
  target_sources(${target_name} PRIVATE ${_arg_SOURCES})

  if (APPLE AND BUILD_WITH_PCH)
    foreach(source IN LISTS _arg_SOURCES)
      if (source MATCHES "^.*\.mm$")
        set_source_files_properties(${source} PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
      endif()
    endforeach()
  endif()

  set_public_headers(${target_name} "${_arg_SOURCES}")

  foreach(file IN LISTS _arg_EXPLICIT_MOC)
    set_explicit_moc(${target_name} "${file}")
  endforeach()

endfunction()

function(extend_qtc_plugin target_name)
  qtc_plugin_enabled(_plugin_enabled ${target_name})
  if (NOT _plugin_enabled)
    return()
  endif()

  extend_qtc_target(${target_name} ${ARGN})
endfunction()

function(extend_qtc_library target_name)
  qtc_library_enabled(_library_enabled ${target_name})
  if (NOT _library_enabled)
    return()
  endif()

  extend_qtc_target(${target_name} ${ARGN})
endfunction()

function(add_qtc_executable name)
  cmake_parse_arguments(_arg "SKIP_INSTALL;SKIP_TRANSLATION;ALLOW_ASCII_CASTS"
    "DESTINATION;COMPONENT"
    "DEFINES;DEPENDS;EXTRA_TRANSLATIONS;INCLUDES;SOURCES;PROPERTIES" ${ARGN})

  if ($_arg_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "add_qtc_executable had unparsed arguments!")
  endif()

  set(default_defines_copy ${DEFAULT_DEFINES})
  if (_arg_ALLOW_ASCII_CASTS)
    list(REMOVE_ITEM default_defines_copy QT_NO_CAST_TO_ASCII QT_RESTRICTED_CAST_FROM_ASCII)
  endif()

  update_cached_list(__QTC_EXECUTABLES "${name}")

  string(TOUPPER "BUILD_EXECUTABLE_${name}" _build_executable_var)
  set(_build_executable_default "ON")
  if (DEFINED ENV{QTC_${_build_executable_var}})
    set(_build_executable_default "$ENV{QTC_${_build_executable_var}}")
  endif()
  set(${_build_executable_var} "${_build_executable_default}" CACHE BOOL "Build executable ${name}.")

  if (NOT ${_build_executable_var})
    return()
  endif()

  set(_DESTINATION "${IDE_LIBEXEC_PATH}")
  if (_arg_DESTINATION)
    set(_DESTINATION "${_arg_DESTINATION}")
  endif()

  set(_EXECUTABLE_PATH "${_DESTINATION}")
  if (APPLE)
    # path of executable might be inside app bundle instead of DESTINATION directly
    cmake_parse_arguments(_prop "" "MACOSX_BUNDLE;OUTPUT_NAME" "" "${_arg_PROPERTIES}")
    if (_prop_MACOSX_BUNDLE)
      set(_BUNDLE_NAME "${name}")
      if (_prop_OUTPUT_NAME)
        set(_BUNDLE_NAME "${_prop_OUTPUT_NAME}")
      endif()
      set(_EXECUTABLE_PATH "${_DESTINATION}/${_BUNDLE_NAME}.app/Contents/MacOS")
    endif()
  endif()

  add_executable("${name}" ${_arg_SOURCES})
  target_include_directories("${name}" PRIVATE "${CMAKE_BINARY_DIR}/src" ${_arg_INCLUDES})
  target_compile_definitions("${name}" PRIVATE ${default_defines_copy} ${TEST_DEFINES} ${_arg_DEFINES} )
  target_link_libraries("${name}" PRIVATE ${_arg_DEPENDS} ${IMPLICIT_DEPENDS})

  set(skip_translation OFF)
  if (_arg_SKIP_TRANSLATION)
    set(skip_translation ON)
  endif()

  file(RELATIVE_PATH relative_lib_path "/${_EXECUTABLE_PATH}" "/${IDE_LIBRARY_PATH}")

  set(build_rpath "${_RPATH_BASE}/${relative_lib_path}")
  set(install_rpath "${_RPATH_BASE}/${relative_lib_path}")
  if (NOT WIN32 AND NOT APPLE)
    file(RELATIVE_PATH relative_qt_path "/${_EXECUTABLE_PATH}" "/${IDE_LIBRARY_BASE_PATH}/Qt/lib")
    file(RELATIVE_PATH relative_plugins_path "/${_EXECUTABLE_PATH}" "/${IDE_PLUGIN_PATH}")
    set(install_rpath "${install_rpath};${_RPATH_BASE}/${relative_qt_path};${_RPATH_BASE}/${relative_plugins_path}")
  endif()

  qtc_output_binary_dir(_output_binary_dir)
  set_target_properties("${name}" PROPERTIES
    BUILD_RPATH "${build_rpath}"
    INSTALL_RPATH "${install_rpath}"
    RUNTIME_OUTPUT_DIRECTORY "${_output_binary_dir}/${_DESTINATION}"
    QT_SKIP_TRANSLATION "${skip_translation}"
    CXX_EXTENSIONS OFF
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
    ${_arg_PROPERTIES}
  )
  append_extra_translations("${name}" "${_arg_EXTRA_TRANSLATIONS}")
  enable_pch(${name})

  if (NOT _arg_SKIP_INSTALL)
    unset(COMPONENT_OPTION)
    if (_arg_COMPONENT)
      set(COMPONENT_OPTION "COMPONENT" "${_arg_COMPONENT}")
    endif()

    install(TARGETS ${name}
      DESTINATION "${_DESTINATION}"
      ${COMPONENT_OPTION}
      OPTIONAL
    )
    update_cached_list(__QTC_INSTALLED_EXECUTABLES
      "${_DESTINATION}/${name}${CMAKE_EXECUTABLE_SUFFIX}")

    install(CODE "
      function(create_qt_conf location base_dir)
        get_filename_component(install_prefix \"\${CMAKE_INSTALL_PREFIX}\" ABSOLUTE)
        file(RELATIVE_PATH qt_conf_binaries
          \"\${install_prefix}/\${location}\"
          \"\${install_prefix}/\${base_dir}\"
        )
        if (NOT qt_conf_binaries)
          set(qt_conf_binaries .)
        endif()
        file(RELATIVE_PATH qt_conf_plugins
          \"\${install_prefix}/\${base_dir}\"
          \"\${install_prefix}/${QT_DEST_PLUGIN_PATH}\"
        )
        file(RELATIVE_PATH qt_conf_qml
          \"\${install_prefix}/\${base_dir}\"
          \"\${install_prefix}/${QT_DEST_QML_PATH}\"
        )
        file(WRITE \"\${CMAKE_INSTALL_PREFIX}/\${location}/qt.conf\"
          \"[Paths]\n\"
          \"Plugins=\${qt_conf_plugins}\n\"
          \"Qml2Imports=\${qt_conf_qml}\n\"
        )
        # For Apple for Qt Creator do not add a Prefix
        if (NOT APPLE OR NOT qt_conf_binaries STREQUAL \"../\")
          file(APPEND \"\${CMAKE_INSTALL_PREFIX}/\${location}/qt.conf\"
            \"Prefix=\${qt_conf_binaries}\n\"
          )
        endif()
        if (WIN32 OR APPLE)
          file(RELATIVE_PATH qt_binaries
            \"\${install_prefix}/\${base_dir}\"
            \"\${install_prefix}/${IDE_BIN_PATH}\"
          )
          if (NOT qt_binaries)
            set(qt_binaries .)
          endif()
          file(APPEND \"\${CMAKE_INSTALL_PREFIX}/\${location}/qt.conf\"
            \"# Needed by QtCreator for qtdiag\n\"
            \"Binaries=\${qt_binaries}\n\")
        endif()
      endfunction()
      if(APPLE)
        create_qt_conf(\"${_EXECUTABLE_PATH}\" \"${IDE_DATA_PATH}/..\")
      elseif (WIN32)
        create_qt_conf(\"${_EXECUTABLE_PATH}\" \"${IDE_APP_PATH}\")
      else()
        create_qt_conf(\"${_EXECUTABLE_PATH}\" \"${IDE_LIBRARY_BASE_PATH}/Qt\")
      endif()
      "
      COMPONENT Dependencies
      EXCLUDE_FROM_ALL
     )

  endif()
endfunction()

function(extend_qtc_executable name)
  if (NOT (name IN_LIST __QTC_EXECUTABLES))
    message(FATAL_ERROR "extend_qtc_executable: Unknown executable target \"${name}\"")
  endif()
  if (TARGET ${name})
    extend_qtc_target(${name} ${ARGN})
  endif()
endfunction()

function(add_qtc_test name)
  cmake_parse_arguments(_arg "GTEST" "" "DEFINES;DEPENDS;INCLUDES;SOURCES" ${ARGN})

  foreach(dependency ${_arg_DEPENDS})
    if (NOT TARGET ${dependency} AND NOT _arg_GTEST)
      if (WITH_DEBUG_CMAKE)
        message(STATUS  "'${dependency}' is not a target")
      endif()
      return()
    endif()
  endforeach()

  if ($_arg_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "add_qtc_test had unparsed arguments!")
  endif()

  update_cached_list(__QTC_TESTS "${name}")

  set(TEST_DEFINES SRCDIR="${CMAKE_CURRENT_SOURCE_DIR}")
  file(RELATIVE_PATH _RPATH "/${IDE_BIN_PATH}" "/${IDE_LIBRARY_PATH}")

  add_executable(${name} ${_arg_SOURCES})

  add_qtc_depends(${name}
    PRIVATE ${_arg_DEPENDS} ${IMPLICIT_DEPENDS}
  )

  target_include_directories(${name} PRIVATE "${CMAKE_BINARY_DIR}/src" ${_arg_INCLUDES})
  target_compile_definitions(${name} PRIVATE ${_arg_DEFINES} ${TEST_DEFINES} ${DEFAULT_DEFINES})

  set_target_properties(${name} PROPERTIES
    BUILD_RPATH "${_RPATH_BASE}/${_RPATH}"
    INSTALL_RPATH "${_RPATH_BASE}/${_RPATH}"
  )
  enable_pch(${name})

  if (NOT _arg_GTEST)
    add_test(NAME ${name} COMMAND ${name})
    finalize_test_setup(${name})
  endif()
endfunction()

function(finalize_qtc_gtest test_name)
  if (NOT TARGET ${test_name})
    return()
  endif()
  get_target_property(test_sources ${test_name} SOURCES)
  include(GoogleTest)
  gtest_add_tests(TARGET ${test_name} SOURCES ${test_sources} TEST_LIST test_list)

  foreach(test IN LISTS test_list)
    finalize_test_setup(${test})
  endforeach()
endfunction()

# This is the CMake equivalent of "RESOURCES = $$files()" from qmake
function(qtc_glob_resources)
  cmake_parse_arguments(_arg "" "QRC_FILE;ROOT;GLOB" "" ${ARGN})

  file(GLOB_RECURSE fileList RELATIVE "${_arg_ROOT}" "${_arg_ROOT}/${_arg_GLOB}")
  set(qrcData "<RCC><qresource>\n")
  foreach(file IN LISTS fileList)
    string(APPEND qrcData "  <file alias=\"${file}\">${_arg_ROOT}/${file}</file>\n")
  endforeach()
  string(APPEND qrcData "</qresource></RCC>")
  file(WRITE "${_arg_QRC_FILE}" "${qrcData}")
endfunction()

function(qtc_copy_to_builddir custom_target_name)
  cmake_parse_arguments(_arg "CREATE_SUBDIRS" "DESTINATION" "FILES;DIRECTORIES" ${ARGN})
  set(timestampFiles)

  qtc_output_binary_dir(_output_binary_dir)

  foreach(srcFile ${_arg_FILES})
    string(MAKE_C_IDENTIFIER "${srcFile}" destinationTimestampFilePart)
    set(destinationTimestampFileName "${CMAKE_CURRENT_BINARY_DIR}/.${destinationTimestampFilePart}_timestamp")
    list(APPEND timestampFiles "${destinationTimestampFileName}")

    if (IS_ABSOLUTE "${srcFile}")
      set(srcPath "")
    else()
      get_filename_component(srcPath "${srcFile}" DIRECTORY)
    endif()

    add_custom_command(OUTPUT "${destinationTimestampFileName}"
      COMMAND "${CMAKE_COMMAND}" -E make_directory "${_output_binary_dir}/${_arg_DESTINATION}/${srcPath}"
      COMMAND "${CMAKE_COMMAND}" -E copy "${srcFile}" "${_output_binary_dir}/${_arg_DESTINATION}/${srcPath}"
      COMMAND "${CMAKE_COMMAND}" -E touch "${destinationTimestampFileName}"
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      COMMENT "Copy ${srcFile} into build directory"
      DEPENDS "${srcFile}"
      VERBATIM
    )
  endforeach()

  foreach(srcDirectory ${_arg_DIRECTORIES})
    string(MAKE_C_IDENTIFIER "${srcDirectory}" destinationTimestampFilePart)
    set(destinationTimestampFileName "${CMAKE_CURRENT_BINARY_DIR}/.${destinationTimestampFilePart}_timestamp")
    list(APPEND timestampFiles "${destinationTimestampFileName}")
    set(destinationDirectory "${_output_binary_dir}/${_arg_DESTINATION}")

    if(_arg_CREATE_SUBDIRS)
      set(destinationDirectory "${destinationDirectory}/${srcDirectory}")
    endif()

    file(GLOB_RECURSE filesToCopy "${srcDirectory}/*")
    add_custom_command(OUTPUT "${destinationTimestampFileName}"
      COMMAND "${CMAKE_COMMAND}" -E copy_directory "${srcDirectory}" "${destinationDirectory}"
      COMMAND "${CMAKE_COMMAND}" -E touch "${destinationTimestampFileName}"
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      COMMENT "Copy ${srcDirectory}/ into build directory"
      DEPENDS ${filesToCopy}
      VERBATIM
    )
  endforeach()

  add_custom_target("${custom_target_name}" ALL DEPENDS ${timestampFiles})
endfunction()
