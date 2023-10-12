# https://cmake.org/cmake/help/latest/manual/cmake-developer.7.html#a-sample-find-module

#[=======================================================================[.rst:
Findstdvector
-------

Finds the stdvector library.

Imported Targets
^^^^^^^^^^^^^^^^

This module provides the following imported targets, if found:

``stdvector::stdvector``
The stdvector library

Result Variables
^^^^^^^^^^^^^^^^

This will define the following variables:

``stdvector_FOUND``
True if the system has the stdvector library.
``stdvector_INCLUDE_DIRS``
Include directories needed to use stdvector.

Cache Variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``stdvector_INCLUDE_DIR``
The directory containing ``vector``.

#]=======================================================================]

find_path(stdvector_INCLUDE_DIR
  NAMES vector
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(stdvector
  FOUND_VAR stdvector_FOUND
  REQUIRED_VARS
  stdvector_INCLUDE_DIR
)

if(stdvector_FOUND)
  set(stdvector_INCLUDE_DIRS ${stdvector_INCLUDE_DIR})
endif()

if(stdvector_FOUND AND NOT TARGET stdvector::stdvector)
  add_library(stdvector::stdvector UNKNOWN IMPORTED)
  set_target_properties(stdvector::stdvector PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${stdvector_INCLUDE_DIR}"
  )
endif()

mark_as_advanced(
  stdvector_INCLUDE_DIR
)
