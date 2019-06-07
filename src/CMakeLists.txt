# Not in the main CMakeLists.txt file because some tests fail if we have the flags set as default
list(APPEND DEFAULT_DEFINES
  QT_CREATOR
  QT_NO_JAVA_STYLE_ITERATORS
  QT_NO_CAST_TO_ASCII QT_RESTRICTED_CAST_FROM_ASCII
  QT_DISABLE_DEPRECATED_BEFORE=0x050900
  QT_USE_FAST_OPERATOR_PLUS
  QT_USE_FAST_CONCATENATION
)

set(__QTC_PLUGINS "" CACHE INTERNAL "*** Internal ***" FORCE)
set(__QTC_LIBRARIES "" CACHE INTERNAL "*** Internal ***" FORCE)
set(__QTC_EXECUTABLES "" CACHE INTERNAL "*** Internal ***" FORCE)
set(__QTC_TESTS "" CACHE INTERNAL "*** Internal ***" FORCE)

function(update_cached_list name value)
  set(_tmp_list "${${name}}")
  list(APPEND _tmp_list "${value}")
  set("${name}" "${_tmp_list}" CACHE INTERNAL "*** Internal ***" FORCE)
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
  set_property(SOURCE "${file}" PROPERTY SKIP_AUTOMOC ON)
  qt5_wrap_cpp(file_moc "${file}")
  target_sources(${target_name} PRIVATE "${file_moc}")
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

function(add_qtc_library name)
  cmake_parse_arguments(_arg "STATIC;OBJECT" ""
    "DEFINES;DEPENDS;INCLUDES;PUBLIC_DEFINES;PUBLIC_DEPENDS;PUBLIC_INCLUDES;SOURCES;EXPLICIT_MOC;SKIP_AUTOMOC;PROPERTIES" ${ARGN}
  )

  if (${_arg_UNPARSED_ARGUMENTS})
    message(FATAL_ERROR "add_qtc_library had unparsed arguments")
  endif()

  update_cached_list(__QTC_LIBRARIES "${name}")

  compare_sources_with_existing_disk_files(${name} "${_arg_SOURCES}")

  set(library_type SHARED)
  if (_arg_STATIC)
    set(library_type STATIC)
  endif()
  if (_arg_OBJECT)
    set(library_type OBJECT)
  endif()

  # Do not just build libraries...
  add_library(${name} ${library_type} EXCLUDE_FROM_ALL ${_arg_SOURCES})

  if (${name} MATCHES "^[^0-9]+")
    string(TOUPPER "${name}_LIBRARY" EXPORT_SYMBOL)
  endif()

  if (WITH_TESTS)
    set(TEST_DEFINES WITH_TESTS SRCDIR="${CMAKE_CURRENT_SOURCE_DIR}")
  endif()

  target_include_directories(${name}
    PRIVATE ${_arg_INCLUDES}
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/.." ${_arg_PUBLIC_INCLUDES}
  )
  target_compile_definitions(${name}
    PRIVATE ${EXPORT_SYMBOL} ${DEFAULT_DEFINES} ${_arg_DEFINES} ${TEST_DEFINES}
    PUBLIC ${_arg_PUBLIC_DEFINES}
  )

  add_qtc_depends(${name}
    PRIVATE ${_arg_DEPENDS} ${_TEST_DEPENDS}
    PUBLIC ${_arg_PUBLIC_DEPENDS}
  )

  foreach(file IN LISTS _arg_EXPLICIT_MOC)
    set_explicit_moc(${name} "${file}")
  endforeach()

  foreach(file IN LISTS _arg_SKIP_AUTOMOC)
    set_property(SOURCE ${file} PROPERTY SKIP_AUTOMOC ON)
  endforeach()

  set_target_properties(${name} PROPERTIES
    SOURCES_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
    VERSION "${PROJECT_VERSION}"
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
    BUILD_RPATH "${_LIB_RPATH}"
    INSTALL_RPATH "${_LIB_RPATH}"
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/${IDE_BIN_PATH}"
    LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/${IDE_LIBRARY_PATH}"
    ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/${IDE_LIBRARY_PATH}"
    ${_arg_PROPERTIES}
  )

  if (NOT (${library_type} STREQUAL "OBJECT" AND CMAKE_VERSION VERSION_LESS 3.14))
    install(TARGETS ${name}
      RUNTIME DESTINATION "${IDE_BIN_PATH}"
      LIBRARY DESTINATION "${IDE_LIBRARY_PATH}"
      ARCHIVE DESTINATION "${IDE_LIBRARY_PATH}"
    )
  endif()

endfunction(add_qtc_library)

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

function(add_qtc_plugin target_name)
  cmake_parse_arguments(_arg
    "EXPERIMENTAL;SKIP_DEBUG_CMAKE_FILE_CHECK;SKIP_INSTALL"
    "VERSION;COMPAT_VERSION;PLUGIN_JSON_IN;PLUGIN_PATH;PLUGIN_NAME;OUTPUT_NAME"
    "CONDITION;DEPENDS;PUBLIC_DEPENDS;DEFINES;INCLUDES;PUBLIC_INCLUDES;PLUGIN_DEPENDS;PLUGIN_RECOMMENDS;SOURCES;EXPLICIT_MOC"
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

  if (NOT _arg_CONDITION)
    set(_arg_CONDITION ON)
    set(_extra_text "")
  else()
    string(REPLACE ";" " " _contents "${_arg_CONDITION}")
    set(_extra_text "with CONDITION ${_contents}")
  endif()

  string(TOUPPER "BUILD_PLUGIN_${target_name}" _build_plugin_var)
  set(_build_plugin_default "ON")
  if (DEFINED ENV{QTC_${_build_plugin_var}})
    set(_build_plugin_default "$ENV{QTC_${_build_plugin_var}}")
  endif()
  set(${_build_plugin_var} "${_build_plugin_default}" CACHE BOOL "Build plugin ${name}.")

  if ((${_arg_CONDITION}) AND ${_build_plugin_var})
    set(_plugin_enabled ON)
  else()
    set(_plugin_enabled OFF)
  endif()

  add_feature_info("Plugin ${name}" _plugin_enabled "${_extra_text}")
  if (NOT _plugin_enabled)
    return()
  endif()

  ### Generate plugin.json file:
  if (NOT _arg_VERSION)
    set(_arg_VERSION ${PROJECT_VERSION})
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
    get_property(_v TARGET "${i}" PROPERTY _arg_VERSION)
    string(APPEND _arg_DEPENDENCY_STRING
      "        { \"Name\" : \"${i}\", \"Version\" : \"${_v}\" }"
    )
  endforeach(i)
  string(REPLACE "}        {" "},\n        {"
    _arg_DEPENDENCY_STRING "${_arg_DEPENDENCY_STRING}"
  )
  foreach(i IN LISTS ${_arg_RECOMMENDS})
    get_property(_v TARGET "${i}" PROPERTY _arg_VERSION)
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
    string(REPLACE "$$dependencyList" "\${IDE_PLUGIN_DEPENDENCY_STRING}" plugin_json_in ${plugin_json_in})
    if(_arg_PLUGIN_JSON_IN)
        #e.g. UPDATEINFO_EXPERIMENTAL_STR=true
        string(REGEX REPLACE "=.*$" "" json_key ${_arg_PLUGIN_JSON_IN})
        string(REGEX REPLACE "^.*=" "" json_value ${_arg_PLUGIN_JSON_IN})
        string(REPLACE "$$${json_key}" "${json_value}" plugin_json_in ${plugin_json_in})
    endif()
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/${name}.json.cmakein" ${plugin_json_in})

    configure_file("${CMAKE_CURRENT_BINARY_DIR}/${name}.json.cmakein" "${name}.json")
  endif()

  add_library(${target_name} SHARED ${_arg_SOURCES})

  ### Generate EXPORT_SYMBOL
  string(TOUPPER "${name}_LIBRARY" EXPORT_SYMBOL)

  if (WITH_TESTS)
    set(TEST_DEFINES WITH_TESTS SRCDIR="${CMAKE_CURRENT_SOURCE_DIR}")
  endif()

  target_include_directories(${target_name}
    PRIVATE ${_arg_INCLUDES} "${CMAKE_CURRENT_SOURCE_DIR}/.." "${CMAKE_CURRENT_BINARY_DIR}"
      "${CMAKE_BINARY_DIR}/src"
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/.." ${_arg_PUBLIC_INCLUDES}
  )
  target_compile_definitions(${target_name}
    PRIVATE ${EXPORT_SYMBOL} ${DEFAULT_DEFINES} ${_arg_DEFINES} ${TEST_DEFINES}
  )

  add_qtc_depends(${target_name}
    PRIVATE ${_arg_DEPENDS} ${_DEP_PLUGINS} ${_TEST_DEPENDS}
    PUBLIC ${_arg_PUBLIC_DEPENDS}
  )

  set(plugin_dir "${IDE_PLUGIN_PATH}")
  if (_arg_PLUGIN_PATH)
    set(plugin_dir "${_arg_PLUGIN_PATH}")
  endif()

  set_target_properties(${target_name} PROPERTIES
    SOURCES_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
    _arg_DEPENDS "${_arg_PLUGIN_DEPENDS}"
    _arg_VERSION "${_arg_VERSION}"
    BUILD_RPATH "${_PLUGIN_RPATH}"
    INSTALL_RPATH "${_PLUGIN_RPATH}"
    LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/${plugin_dir}"
    ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/${plugin_dir}"
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/${plugin_dir}"
    OUTPUT_NAME "${name}"
    ${_arg_PROPERTIES}
  )

  foreach(file IN LISTS _arg_EXPLICIT_MOC)
    set_explicit_moc(${target_name} "${file}")
  endforeach()

  if (NOT _arg_SKIP_INSTALL)
    install(TARGETS ${target_name}
            LIBRARY DESTINATION "${plugin_dir}"
            ARCHIVE DESTINATION "${plugin_dir}"
            RUNTIME DESTINATION "${plugin_dir}"
    )
  endif()
endfunction()

function(extend_qtc_target target_name)
  cmake_parse_arguments(_arg
    ""
    "SOURCES_PREFIX"
    "CONDITION;DEPENDS;PUBLIC_DEPENDS;DEFINES;INCLUDES;PUBLIC_INCLUDES;SOURCES;EXPLICIT_MOC"
    ${ARGN}
  )

  if (${_arg_UNPARSED_ARGUMENTS})
    message(FATAL_ERROR "extend_qtc_target had unparsed arguments")
  endif()

  if (NOT _arg_CONDITION)
    set(_arg_CONDITION ON)
  endif()
  if (NOT (${_arg_CONDITION}))
    return()
  endif()

  add_qtc_depends(${target_name}
    PRIVATE ${_arg_DEPENDS}
    PUBLIC ${_arg_PUBLIC_DEPENDS}
  )
  target_compile_definitions(${target_name} PRIVATE ${_arg_DEFINES})
  target_include_directories(${target_name} PRIVATE ${_arg_INCLUDES} PUBLIC ${_arg_PUBLIC_INCLUDES})

  if (_arg_SOURCES_PREFIX)
    foreach(source IN LISTS _arg_SOURCES)
      list(APPEND prefixed_sources "${_arg_SOURCES_PREFIX}/${source}")
    endforeach()
    target_include_directories(${target_name} PUBLIC "${_arg_SOURCES_PREFIX}")
    set(_arg_SOURCES ${prefixed_sources})
  endif()
  target_sources(${target_name} PRIVATE ${_arg_SOURCES})

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

function(add_qtc_executable name)
  cmake_parse_arguments(_arg "SKIP_INSTALL" "DESTINATION" "DEFINES;DEPENDS;INCLUDES;SOURCES;PROPERTIES" ${ARGN})

  if ($_arg_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "add_qtc_executable had unparsed arguments!")
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

  file(RELATIVE_PATH _RELATIVE_LIB_PATH "/${_EXECUTABLE_PATH}" "/${IDE_LIBRARY_PATH}")

  add_executable("${name}" ${_arg_SOURCES})
  target_include_directories("${name}" PRIVATE "${CMAKE_BINARY_DIR}/src" ${_arg_INCLUDES})
  target_compile_definitions("${name}" PRIVATE ${_arg_DEFINES} ${TEST_DEFINES} ${DEFAULT_DEFINES})
  target_link_libraries("${name}" PRIVATE ${_arg_DEPENDS} ${_TEST_DEPENDS})
  set_target_properties("${name}" PROPERTIES
    BUILD_RPATH "${_RPATH_BASE}/${_RELATIVE_LIB_PATH}"
    INSTALL_RPATH "${_RPATH_BASE}/${_RELATIVE_LIB_PATH}"
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/${_DESTINATION}"
    ${_arg_PROPERTIES}
  )

  if (NOT _arg_SKIP_INSTALL)
    install(TARGETS ${name} DESTINATION "${_DESTINATION}")
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

function(fix_test_environment test_name)
  if (WIN32)
    list(APPEND env_path $ENV{PATH})
    list(APPEND env_path ${CMAKE_BINARY_DIR}/${IDE_PLUGIN_PATH})
    list(APPEND env_path ${CMAKE_BINARY_DIR}/${IDE_BIN_PATH})
    list(APPEND env_path $<TARGET_FILE_DIR:Qt5::Test>)
    if (TARGET libclang)
        list(APPEND env_path $<TARGET_FILE_DIR:libclang>)
    endif()

    string(REPLACE "/" "\\" env_path "${env_path}")
    string(REPLACE ";" "\\;" env_path "${env_path}")

    set_tests_properties(${test_name} PROPERTIES ENVIRONMENT "PATH=${env_path}")
  endif()
endfunction()

function(add_qtc_test name)
  cmake_parse_arguments(_arg "GTEST" "" "DEFINES;DEPENDS;INCLUDES;SOURCES" ${ARGN})

  if ($_arg_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "add_qtc_test had unparsed arguments!")
  endif()

  update_cached_list(__QTC_TESTS "${name}")

  set(TEST_DEFINES SRCDIR="${CMAKE_CURRENT_SOURCE_DIR}")
  file(RELATIVE_PATH _RPATH "/${IDE_BIN_PATH}" "/${IDE_LIBRARY_PATH}")

  add_executable(${name} ${_arg_SOURCES})

  add_qtc_depends(${name}
    PRIVATE ${_arg_DEPENDS} ${_TEST_DEPENDS}
  )

  target_include_directories(${name} PRIVATE "${CMAKE_BINARY_DIR}/src" ${_arg_INCLUDES})
  target_compile_definitions(${name} PRIVATE ${_arg_DEFINES} ${TEST_DEFINES} ${DEFAULT_DEFINES})

  set_target_properties(${name} PROPERTIES
    BUILD_RPATH "${_RPATH_BASE}/${_RPATH}"
    INSTALL_RPATH "${_RPATH_BASE}/${_RPATH}"
  )

  if (NOT _arg_GTEST)
    add_test(NAME ${name} COMMAND ${name})
    fix_test_environment(${name})
  endif()
endfunction()

function(finalize_qtc_gtest test_name)
  get_target_property(test_sources ${test_name} SOURCES)
  include(GoogleTest)
  gtest_add_tests(TARGET ${test_name} SOURCES ${test_sources} TEST_LIST test_list)

  foreach(test IN LISTS test_list)
    fix_test_environment(${test})
  endforeach()
endfunction()

add_library(app_version INTERFACE)
target_include_directories(app_version INTERFACE ${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(libs)
add_subdirectory(share)
add_subdirectory(shared)
add_subdirectory(app)
add_subdirectory(plugins)
add_subdirectory(tools)
