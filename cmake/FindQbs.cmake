#.rst:
# FindQbs
# ---------
#
# Try to locate the Qbs library.
# If found, this will define the following variables:
#
# ``QBS_FOUND``
#     True if the qbs library is available
# ``QBS_INCLUDE_DIRS``
#     The qbs include directories
# ``QBSCORE_LIBRARIES``
#     The qbscore library for linking
# ``QBS_INSTALL_DIR``
#     Top level qbs installation directory
#
# If ``QBS_FOUND`` is TRUE, it will also define the following
# imported target:
#
# ``QBS::QBS``
#     The qbs library

find_program(QBS_BINARY NAMES qbs)
if(QBS_BINARY STREQUAL "QBS_BINARY-NOTFOUND")
  set(_QBS_INSTALL_DIR "QBS_INSTALL_DIR-NOTFOUND")
else()
  get_filename_component(_QBS_BIN_DIR "${QBS_BINARY}" DIRECTORY)
  get_filename_component(_QBS_INSTALL_DIR "${_QBS_BIN_DIR}" DIRECTORY)
endif()

set(QBS_INSTALL_DIR "${_QBS_INSTALL_DIR}" CACHE PATH "Qbs install directory")

find_path(QBS_INCLUDE_DIRS NAMES qbs.h PATH_SUFFIXES qbs HINTS "${QBS_INSTALL_DIR}/include")

find_library(QBSCORE_LIBRARIES NAMES qbscore HINTS "${QBS_INSTALL_DIR}/lib")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(QBS DEFAULT_MSG QBSCORE_LIBRARIES QBS_INCLUDE_DIRS)

if(QBS_FOUND AND NOT TARGET Qbs::QbsCore)
  add_library(Qbs::QbsCore UNKNOWN IMPORTED)
  # FIXME: Detect whether QBS_ENABLE_PROJECT_FILE_UPDATES is set in qbscore!
  set_target_properties(Qbs::QbsCore PROPERTIES
                        IMPORTED_LOCATION "${QBSCORE_LIBRARIES}"
                        INTERFACE_INCLUDE_DIRECTORIES "${QBS_INCLUDE_DIRS}"
                        INTERFACE_COMPILE_DEFINITIONS "QBS_ENABLE_PROJECT_FILE_UPDATES")
endif()

mark_as_advanced(QBS_INCLUDE_DIRS QBSCORE_LIBRARIES QBS_INSTALL_DIR)

include(FeatureSummary)
set_package_properties(QBS PROPERTIES
  URL "https://qt.io/qbs"
  DESCRIPTION "QBS build system")

