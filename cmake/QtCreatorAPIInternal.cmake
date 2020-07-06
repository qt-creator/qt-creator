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

file(RELATIVE_PATH _PLUGIN_TO_LIB "/${_IDE_PLUGIN_PATH}" "/${_IDE_LIBRARY_PATH}")
file(RELATIVE_PATH _PLUGIN_TO_QT "/${_IDE_PLUGIN_PATH}" "/${_IDE_LIBRARY_BASE_PATH}/Qt/lib")
file(RELATIVE_PATH _LIB_TO_QT "/${_IDE_LIBRARY_PATH}" "/${_IDE_LIBRARY_BASE_PATH}/Qt/lib")

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
  cmake_parse_arguments(_arg "" "TIMEOUT" "" ${ARGN})
  if (DEFINED _arg_TIMEOUT)
    set(timeout ${_arg_TIMEOUT})
  else()
    set(timeout 5)
  endif()
  # Never translate tests:
  set_tests_properties(${name}
    PROPERTIES
      QT_SKIP_TRANSLATION ON
      TIMEOUT ${timeout}
  )

  if (WIN32)
    list(APPEND env_path $ENV{PATH})
    list(APPEND env_path ${CMAKE_BINARY_DIR}/${_IDE_PLUGIN_PATH})
    list(APPEND env_path ${CMAKE_BINARY_DIR}/${_IDE_BIN_PATH})
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
        set_source_files_properties(
            ${CMAKE_CURRENT_BINARY_DIR}/empty_pch.c
            ${CMAKE_CURRENT_BINARY_DIR}/empty_pch.cpp
            PROPERTIES GENERATED TRUE)

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

function(condition_info varName condition)
  if (NOT ${condition})
    set(${varName} "" PARENT_SCOPE)
  else()
    string(REPLACE ";" " " _contents "${${condition}}")
    set(${varName} "with CONDITION ${_contents}" PARENT_SCOPE)
  endif()
endfunction()

function(extend_qtc_target target_name)
  cmake_parse_arguments(_arg
    ""
    "SOURCES_PREFIX;FEATURE_INFO"
    "CONDITION;DEPENDS;PUBLIC_DEPENDS;DEFINES;PUBLIC_DEFINES;INCLUDES;PUBLIC_INCLUDES;SOURCES;EXPLICIT_MOC;SKIP_AUTOMOC;EXTRA_TRANSLATIONS"
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

  foreach(file IN LISTS _arg_SKIP_AUTOMOC)
    set_property(SOURCE ${file} PROPERTY SKIP_AUTOMOC ON)
  endforeach()

  append_extra_translations(${target_name} "${_arg_EXTRA_TRANSLATIONS}")
endfunction()
