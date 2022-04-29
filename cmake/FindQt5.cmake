#.rst:
# FindQt5
# -------
#
# Qt5 wrapper around Qt6 CMake code.
#

unset(__arguments)
if (Qt5_FIND_QUIETLY)
  list(APPEND __arguments QUIET)
endif()
if (Qt5_FIND_REQUIRED)
  list(APPEND __arguments REQUIRED)
endif()

if (Qt5_FIND_COMPONENTS)
  # for some reason QUIET doesn't really work when passed to the arguments list
  if (Qt5_FIND_QUIETLY)
    list(APPEND __arguments OPTIONAL_COMPONENTS)
  else()
    list(APPEND __arguments COMPONENTS)
  endif()
endif()

find_package(Qt6 ${Qt5_FIND_VERSION} CONFIG COMPONENTS Core QUIET)
if (NOT Qt6_FOUND)
  # remove Core5Compat from components to find in Qt5, but add a dummy target,
  # which unfortunately cannot start with "Qt6::"
  # also remove Tools, where some tools have moved in Qt6, e.g. from Help
  list(REMOVE_ITEM Qt5_FIND_COMPONENTS Core5Compat)
  list(REMOVE_ITEM Qt5_FIND_COMPONENTS Tools)
  find_package(Qt5 ${Qt5_FIND_VERSION} CONFIG ${__arguments} ${Qt5_FIND_COMPONENTS})
  if (NOT TARGET Qt6Core5Compat)
    add_library(Qt6Core5Compat INTERFACE)
  endif()

  # Remove Qt6 from the not found packages in Qt5 mode
  get_property(not_found_packages GLOBAL PROPERTY "PACKAGES_NOT_FOUND")
  if(not_found_packages)
    list(REMOVE_ITEM not_found_packages Qt6)
    set_property(GLOBAL PROPERTY "PACKAGES_NOT_FOUND" "${not_found_packages}")
  endif()
  return()
else()
  # since Qt 6.2 some components are renamed to *Private
  foreach(possible_private_libs DesignerComponents QmlDebug)
      list(FIND Qt5_FIND_COMPONENTS ${possible_private_libs} dcIndex)
      if(dcIndex GREATER_EQUAL 0)
        find_package(Qt6${possible_private_libs}Private CONFIG QUIET)
        if(TARGET Qt6::${possible_private_libs}Private)
          set_property(TARGET Qt6::${possible_private_libs}Private PROPERTY IMPORTED_GLOBAL TRUE)
          add_library(Qt5::${possible_private_libs} ALIAS Qt6::${possible_private_libs}Private)
          list(REMOVE_AT Qt5_FIND_COMPONENTS ${dcIndex})
        endif()
      endif()
  endforeach()
  find_package(Qt6 CONFIG ${__arguments} ${Qt5_FIND_COMPONENTS})
endif()

set(__additional_imported_components ATSPI2_nolink) # Work around QTBUG-97023
foreach(comp IN LISTS Qt5_FIND_COMPONENTS __additional_imported_components)
  if(TARGET Qt6::${comp})
    if (NOT TARGET Qt5::${comp})
      if (NOT QT_FEATURE_static)
        set_property(TARGET Qt6::${comp} PROPERTY IMPORTED_GLOBAL TRUE)
      endif()
      add_library(Qt5::${comp} ALIAS Qt6::${comp})
    endif()
    if (TARGET Qt6::${comp}Private AND NOT TARGET Qt5::${comp}Private)
      if (NOT QT_FEATURE_static)
        set_property(TARGET Qt6::${comp}Private PROPERTY IMPORTED_GLOBAL TRUE)
      endif()
      add_library(Qt5::${comp}Private ALIAS Qt6::${comp}Private)
    endif()
  endif()
endforeach()

# alias Qt6::Core5Compat to Qt6Core5Compat to make consistent with Qt5 path
if (TARGET Qt6::Core5Compat AND NOT TARGET Qt6Core5Compat)
  add_library(Qt6Core5Compat ALIAS Qt6::Core5Compat)
endif()

set(Qt5_FOUND ${Qt6_FOUND})
set(Qt5_VERSION ${Qt6_VERSION})

foreach(tool qmake lrelease lupdate moc rcc qhelpgenerator)
  if (TARGET Qt6::${tool} AND NOT TARGET Qt5::${tool})
    add_executable(Qt5::${tool} IMPORTED GLOBAL)
    get_target_property(imported_location Qt6::${tool} IMPORTED_LOCATION)
    # handle separate tools for each configuration
    if (NOT imported_location)
      get_target_property(imported_location Qt6::${tool} IMPORTED_LOCATION_RELEASE)
    endif()
    set_target_properties(Qt5::${tool} PROPERTIES IMPORTED_LOCATION "${imported_location}")
  endif()
endforeach()

if (NOT DEFINED qt5_wrap_cpp)
  function(qt5_wrap_cpp outfiles)
    qt6_wrap_cpp(${outfiles} ${ARGN})
    set(${outfiles} ${${outfiles}} PARENT_SCOPE)
  endfunction()
endif()
