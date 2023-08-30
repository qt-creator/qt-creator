add_subdirectory(3rdparty)

add_subdirectory(nanotrace)
add_subdirectory(advanceddockingsystem)
add_subdirectory(aggregation)
add_subdirectory(cplusplus)
add_subdirectory(extensionsystem)
add_subdirectory(glsl)
add_subdirectory(languageserverprotocol)
add_subdirectory(languageutils)
add_subdirectory(modelinglib)
add_subdirectory(qmldebug)
add_subdirectory(qmleditorwidgets)
add_subdirectory(qmljs)
add_subdirectory(qtcreatorcdbext)
add_subdirectory(utils)
add_subdirectory(solutions)
add_subdirectory(tracing)

if (WITH_QMLDESIGNER)
  add_subdirectory(sqlite)
  add_subdirectory(qmlpuppetcommunication)
endif()

if (WITH_TESTS)
 add_subdirectory(googletest)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/qlitehtml/src/CMakeLists.txt)
  option(BUILD_LIBRARY_QLITEHTML "Build library qlitehtml." ${BUILD_LIBRARIES_BY_DEFAULT})
  set(QLITEHTML_VERSION_COMPAT ${IDE_VERSION_COMPAT} CACHE STRING "")
  if(BUILD_LIBRARY_QLITEHTML)
    set(QLITEHTML_LIBRARY_ARCHIVE_PATH ${IDE_LIBRARY_ARCHIVE_PATH})
    set(QLITEHTML_EXPORT QtCreator)
    set(QLITEHTML_DEVEL_COMPONENT Devel)
    set(QLITEHTML_DEVEL_EXCLUDE_FROM_ALL ON)
    set(QLITEHTML_HEADER_PATH "${IDE_HEADER_INSTALL_PATH}/src/lib/qlitehtml")
    if (QTC_STATIC_BUILD)
      set(QLITEHTML_LIBRARY_TYPE STATIC)
    else()
      set(QLITEHTML_BIN_PATH ${IDE_BIN_PATH})
      set(QLITEHTML_LIBRARY_PATH ${IDE_LIBRARY_PATH})
    endif()
    set(QT_VERSION_MAJOR ${Qt6_VERSION_MAJOR})
    option(BUILD_TESTING "Build litehtml tests" OFF) # otherwise litehtml downloads googletest
    add_subdirectory(qlitehtml/src)
  endif()
  if(TARGET qlitehtml)
    target_compile_definitions(qlitehtml PRIVATE ${DEFAULT_DEFINES}) # for precompiled headers
    enable_pch(qlitehtml)
    #doesn't work: TARGET 'qlitehtml' was not created in this directory.
    #qtc_enable_separate_debug_info(qlitehtml "${IDE_LIBRARY_PATH}")
    qtc_output_binary_dir(_output_binary_dir)
    set_target_properties(qlitehtml PROPERTIES
      BUILD_RPATH "${_LIB_RPATH};${CMAKE_BUILD_RPATH}"
      INSTALL_RPATH "${_LIB_RPATH};${CMAKE_INSTALL_RPATH}"
      RUNTIME_OUTPUT_DIRECTORY "${_output_binary_dir}/${IDE_BIN_PATH}"
      LIBRARY_OUTPUT_DIRECTORY "${_output_binary_dir}/${IDE_LIBRARY_PATH}"
      ARCHIVE_OUTPUT_DIRECTORY "${_output_binary_dir}/${IDE_LIBRARY_ARCHIVE_PATH}"
    )
  endif()
endif()
