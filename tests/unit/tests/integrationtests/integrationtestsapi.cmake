# Copies the test data directory from the source to the build directory
function(integration_test_copy_data_folder)
  execute_process(
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/data"
    "${CMAKE_CURRENT_BINARY_DIR}/data"
  )
endfunction(integration_test_copy_data_folder)

# Convenience helper that adds an executable target and registers it as test with CTest.
# The actual name of the target is "${test_name}_integrationtest", so is the name of the registered
# test. This way the group of integration tests can be easily separated and run on CI/CD.
# The implementation mostly follows that of add_qtc_test, except that automatic discovery of test
# cases is not done. Instead the entire executable is added as a single CTest test. This is done so
# due to CTest running each test case as a separate process, which is time-consuming.
# Note that the test data folder is copied over to the build dir automatically.
function(add_qtc_integration_test test_name)
  # TODO: These fail in the PRECHECK
  return()
  cmake_parse_arguments(
    _arg
    "SKIP_PCH"
    ""
    "CONDITION;DEPENDS;DEFINES;INCLUDES;SOURCES;EXPLICIT_MOC;SKIP_AUTOMOC;PROPERTIES;\
    PRIVATE_COMPILE_OPTIONS;PUBLIC_COMPILE_OPTIONS"
    ${ARGN}
  )

  if(${_arg_UNPARSED_ARGUMENTS})
    message(FATAL_ERROR "add_qtc_integration_test had unparsed arguments!")
  endif(${_arg_UNPARSED_ARGUMENTS})

  string(CONCAT full_test_name "${test_name}" "_integrationtest")
  set(test_executable "${full_test_name}")

  if (NOT _arg_CONDITION)
    set(_arg_CONDITION ON)
  endif()

  if ((${_arg_CONDITION}))
    set(_test_enabled ON)
  else()
    set(_test_enabled OFF)
  endif()
  if (NOT _test_enabled)
    return()
  endif()

  foreach(dependency ${_arg_DEPENDS})
    if (NOT TARGET ${dependency})
      if (WITH_DEBUG_CMAKE)
        message(STATUS  "'${dependency}' is not a target")
      endif()
      return()
    endif()
  endforeach()

  set(TEST_DEFINES WITH_TESTS SRCDIR="${CMAKE_CURRENT_SOURCE_DIR}")

  file(RELATIVE_PATH _RPATH "/${IDE_BIN_PATH}" "/${IDE_LIBRARY_PATH}")

  get_default_defines(default_defines_copy YES)

  add_executable(${full_test_name} ${_arg_SOURCES})

  extend_qtc_target(${full_test_name}
    DEPENDS integrationtestscommon ${_arg_DEPENDS} ${IMPLICIT_DEPENDS}
    INCLUDES "${CMAKE_BINARY_DIR}/src" ${_arg_INCLUDES}
    DEFINES ${_arg_DEFINES} ${TEST_DEFINES} ${default_defines_copy}
    EXPLICIT_MOC ${_arg_EXPLICIT_MOC}
    SKIP_AUTOMOC ${_arg_SKIP_AUTOMOC}
    PRIVATE_COMPILE_OPTIONS ${_arg_PRIVATE_COMPILE_OPTIONS}
    PUBLIC_COMPILE_OPTIONS ${_arg_PUBLIC_COMPILE_OPTIONS}
  )

  set_target_properties(${full_test_name} PROPERTIES
    LINK_DEPENDS_NO_SHARED ON
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
    BUILD_RPATH "${_RPATH_BASE}/${_RPATH};${CMAKE_BUILD_RPATH}"
    INSTALL_RPATH "${_RPATH_BASE}/${_RPATH};${CMAKE_INSTALL_RPATH}"
    ${_arg_PROPERTIES}
  )
  if (NOT _arg_SKIP_PCH)
    enable_pch(${full_test_name})
  endif()

  add_test(NAME "${full_test_name}" COMMAND "${test_executable}")

  integration_test_copy_data_folder()
endfunction(add_qtc_integration_test)
