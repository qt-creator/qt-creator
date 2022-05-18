#.rst:
# FindCrashpad
# ------------
# Locate Crashpad sources and build
#
# The ``CRASHPAD_BUILD_DIR``, ``CRASHPAD_SOURCE_DIR``, and ``CRASHPAD_OBJECT_DIR``
# (APPLE-only) variables can be used as additional hints.
#
# If you built crashpad with the default setup in "out/Default" within the
# source directory, you can set the ``Crashpad_ROOT`` variable to the source
# directory, or add it to ``CMAKE_PREFIX_PATH``.
#
# If found, this will define the following variables:
#
# ``Crashpad_FOUND``
#     True if the necessary Crashpad files have been found
#
# ``Crashpad::Crashpad``
#     Imported target to build against
#
# ``CRASHPAD_BIN_DIR``
#     Directory where the crashpad_handler executable is found

if(TARGET Crashpad::Crashpad)
  set(Crashpad_FOUND TRUE)
  return()
endif()

find_path(CRASHPAD_BIN_DIR
  NAMES crashpad_handler crashpad_handler.exe
  PATH_SUFFIXES out/Default
  HINTS
    "${CRASHPAD_BUILD_DIR}"
    "${CMAKE_PREFIX_PATH}"
)

find_path(CRASHPAD_INCLUDE_DIR
  NAMES client/crashpad_client.h
  HINTS
    "${CRASHPAD_SOURCE_DIR}"
    "${CMAKE_PREFIX_PATH}"
)

find_path(CRASHPAD_LIB_DIR
  NAMES util/libutil.a util/util.lib
  PATH_SUFFIXES out/Default/obj
  HINTS
    "${CRASHPAD_BIN_DIR}/obj"
    "${CMAKE_PREFIX_PATH}"
)

find_path(CRASHPAD_GEN_DIR
    NAMES build/chromeos_buildflags.h
    PATH_SUFFIXES gen
    HINTS
      "${CRASHPAD_BIN_DIR}"
      "${CMAKE_PREFIX_PATH}"
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Crashpad DEFAULT_MSG
  CRASHPAD_BIN_DIR CRASHPAD_INCLUDE_DIR CRASHPAD_LIB_DIR
)

if(Crashpad_FOUND)
  add_library(Crashpad::Crashpad UNKNOWN IMPORTED)
  target_include_directories(Crashpad::Crashpad INTERFACE
    "${CRASHPAD_INCLUDE_DIR}"
    "${CRASHPAD_INCLUDE_DIR}/third_party/mini_chromium/mini_chromium"
    "${CRASHPAD_GEN_DIR}")
  if(WIN32)
    target_link_libraries(Crashpad::Crashpad INTERFACE
      "${CRASHPAD_LIB_DIR}/third_party/mini_chromium/mini_chromium/base/base.lib"
      "${CRASHPAD_LIB_DIR}/util/util.lib"
      "${CRASHPAD_LIB_DIR}/client/client.lib"
      advapi32)
    set_target_properties(Crashpad::Crashpad PROPERTIES
      IMPORTED_LOCATION "${CRASHPAD_LIB_DIR}/client/client.lib")
  elseif(APPLE)
    find_library(FWbsm bsm)
    find_library(FWAppKit AppKit)
    find_library(FWIOKit IOKit)
    find_library(FWSecurity Security)
    target_link_libraries(Crashpad::Crashpad INTERFACE
      "${CRASHPAD_LIB_DIR}/third_party/mini_chromium/mini_chromium/base/libbase.a"
      "${CRASHPAD_LIB_DIR}/util/libutil.a"
      "${CRASHPAD_LIB_DIR}/util/libmig_output.a"
      "${CRASHPAD_LIB_DIR}/client/libclient.a"
      "${CRASHPAD_LIB_DIR}/client/libcommon.a"
      ${FWbsm} ${FWAppKit} ${FWIOKit} ${FWSecurity})
    set_target_properties(Crashpad::Crashpad PROPERTIES
      IMPORTED_LOCATION "${CRASHPAD_LIB_DIR}/client/libclient.a")
  elseif(UNIX)
    # TODO: Crashpad is not well supported on linux currently
    target_link_libraries(Crashpad::Crashpad INTERFACE
      "${CRASHPAD_LIB_DIR}/third_party/mini_chromium/mini_chromium/base/libbase.a"
      "${CRASHPAD_LIB_DIR}/util/libutil.a"
      "${CRASHPAD_LIB_DIR}/client/libclient.a")
    set_target_properties(Crashpad::Crashpad PROPERTIES
      IMPORTED_LOCATION "${CRASHPAD_LIB_DIR}/client/libclient.a")
  endif()
endif()
