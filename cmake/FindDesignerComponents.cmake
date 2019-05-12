#.rst:
# FindDesignerComponents
# ---------
#
# Try to locate the DesignerComponents library.
# If found, this will define the following variables:
#
# ``DesignerComponents_FOUND``
#     True if the DesignerComponents library is available
# ``DesignerComponents_INCLUDE_DIRS``
#     The DesignerComponents include directories
# ``DesignerComponents_LIBRARIES``
#     The DesignerComponentscore library for linking
# ``DesignerComponents_INSTALL_DIR``
#     Top level DesignerComponents installation directory
#
# If ``DesignerComponents_FOUND`` is TRUE, it will also define the following
# imported target:
#
# ``Qt5::DesignerComponents``
#     The DesignerComponents library

find_package(Qt5Designer QUIET)
if (NOT Qt5Designer_FOUND)
    set(DesignerComponents_FOUND OFF)
    return()
endif()


find_path(DesignerComponents_INCLUDE_DIRS NAMES qtdesignercomponentsversion.h PATH_SUFFIXES QtDesignerComponents HINTS ${Qt5Designer_INCLUDE_DIRS})

find_library(DesignerComponents_LIBRARIES NAMES Qt5DesignerComponents)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(DesignerComponents DEFAULT_MSG
  DesignerComponents_LIBRARIES DesignerComponents_INCLUDE_DIRS)

if(DesignerComponents_FOUND AND NOT TARGET DesignerComponents::DesignerComponents)
  add_library(Qt5::DesignerComponents UNKNOWN IMPORTED)
  set_target_properties(Qt5::DesignerComponents PROPERTIES
                        IMPORTED_LOCATION "${DesignerComponents_LIBRARIES}"
                        INTERFACE_INCLUDE_DIRECTORIES "${DesignerComponents_INCLUDE_DIRS}")
endif()

mark_as_advanced(DesignerComponents_INCLUDE_DIRS DesignerComponents_LIBRARIES)

include(FeatureSummary)
set_package_properties(DesignerComponents PROPERTIES
  URL "https://qt.io/"
  DESCRIPTION "Qt5 (Widget) DesignerComponents library")

