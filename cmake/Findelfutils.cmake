#.rst:
# Findelfutils
# -----------------
#
# Try to locate the elfutils development installation
#
# On Debian-like systems, you can install libdw-dev.
#
# Otherwise, the ``ELFUTILS_INSTALL_DIR`` (CMake or Environment) variable
# can be used to pinpoint the elfutils binary installation
#
# If found, this will define the following variables:
#
# ``elfutils_FOUND``
#     True if the elfutils header and library files have been found
#
# ``elfutils::dw``
# ``elfutils::elf``
#     Imported targets containing the includes and libraries need to build
#     against
#

if (TARGET elfutils::dw AND TARGET elfutils::elf)
  set(elfutils_FOUND TRUE)
  return()
endif()

find_path(ELFUTILS_INCLUDE_DIR
  NAMES libdwfl.h elfutils/libdwfl.h
  PATH_SUFFIXES include
  HINTS
    "${ELFUTILS_INSTALL_DIR}" ENV ELFUTILS_INSTALL_DIR "${CMAKE_PREFIX_PATH}"
)

foreach(lib dw elf eu_compat)
  find_library(ELFUTILS_LIB_${lib}
    NAMES ${lib}
    PATH_SUFFIXES lib
    HINTS
      "${ELFUTILS_INSTALL_DIR}" ENV ELFUTILS_INSTALL_DIR "${CMAKE_PREFIX_PATH}"
  )
endforeach()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(elfutils
  DEFAULT_MSG
    ELFUTILS_INCLUDE_DIR ELFUTILS_LIB_dw ELFUTILS_LIB_elf
)

if(elfutils_FOUND)
  foreach(lib dw elf)
    if (NOT TARGET elfutils::${lib})
      add_library(elfutils::${lib} UNKNOWN IMPORTED)
      set_target_properties(elfutils::${lib}
        PROPERTIES
          IMPORTED_LOCATION "${ELFUTILS_LIB_${lib}}"
          INTERFACE_INCLUDE_DIRECTORIES
            "${ELFUTILS_INCLUDE_DIR};${ELFUTILS_INCLUDE_DIR}/elfutils"
      )
      if (ELFUTILS_LIB_eu_compat)
        target_link_libraries(elfutils::${lib} INTERFACE ${ELFUTILS_LIB_eu_compat})
      endif()
    endif()
  endforeach()
else()
  message(STATUS "                        (set ELFUTILS_INSTALL_DIR, or install libdw-dev on a Debian-like system)")
endif()

mark_as_advanced(ELFUTILS_INCLUDE_DIR ELFUTILS_LIB_elf ELFUTILS_LIB_dw)

include(FeatureSummary)
set_package_properties(elfutils PROPERTIES
  URL "https://sourceware.org/elfutils/"
  DESCRIPTION "a collection of utilities and libraries to read, create and modify ELF binary files")
