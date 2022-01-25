#
# Internal Qt Creator variable reference
#
foreach(qtcreator_var
    QT_QMAKE_EXECUTABLE CMAKE_PREFIX_PATH CMAKE_C_COMPILER CMAKE_CXX_COMPILER
    CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELWITHDEBINFO)
  set(__just_reference_${qtcreator_var} ${${qtcreator_var}})
endforeach()

option(QT_CREATOR_SOURCE_GROUPS "Qt Creator source groups extensions" ON)
if (QT_CREATOR_SOURCE_GROUPS)
  source_group("Resources" REGULAR_EXPRESSION "\\.(pdf|plist|png|jpeg|jpg|storyboard|xcassets|qrc|svg|gif|ico|webp)$")
  source_group("Forms" REGULAR_EXPRESSION "\\.(ui)$")
  source_group("State charts" REGULAR_EXPRESSION "\\.(scxml)$")
  source_group("Source Files" REGULAR_EXPRESSION
    "\\.(C|F|M|c|c\\+\\+|cc|cpp|mpp|cxx|ixx|cppm|ccm|cxxm|c\\+\\+m|cu|f|f90|for|fpp|ftn|m|mm|rc|def|r|odl|idl|hpj|bat|qml|js)$"
  )
endif()

#
# Set a better default value for CMAKE_INSTALL_PREFIX
#
function(qtc_modify_default_install_prefix)
  # If at configure time the user didn't specify a CMAKE_INSTALL_PREFIX variable
  # Modules/CMakeGenericSystem.cmake will set a default value
  # to CMAKE_INSTALL_PREFIX and set CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT to ON

  # In practice there are cases when CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT is
  # set to ON and a custom CMAKE_INSTALL_PREFIX is set

  # Do the original CMAKE_INSTALL_PREFIX detection
  if(CMAKE_HOST_UNIX)
    set(original_cmake_install_prefix "/usr/local")
  else()
    GetDefaultWindowsPrefixBase(CMAKE_GENERIC_PROGRAM_FILES)
    set(original_cmake_install_prefix
      "${CMAKE_GENERIC_PROGRAM_FILES}/${PROJECT_NAME}")
    unset(CMAKE_GENERIC_PROGRAM_FILES)
  endif()

  # When the user code didn't modify the CMake set CMAKE_INSTALL_PREFIX
  # then set the "/tmp" better value for CMAKE_INSTALL_PREFIX
  if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT AND
      CMAKE_INSTALL_PREFIX STREQUAL "${original_cmake_install_prefix}")
    set_property(CACHE CMAKE_INSTALL_PREFIX PROPERTY VALUE "/tmp")
  endif()
endfunction()
if (CMAKE_VERSION GREATER_EQUAL "3.19")
  cmake_language(DEFER CALL qtc_modify_default_install_prefix)
endif()

# Store the C/C++ object output extension
if (CMAKE_VERSION GREATER_EQUAL "3.19")
  cmake_language(DEFER CALL set CMAKE_C_OUTPUT_EXTENSION "${CMAKE_C_OUTPUT_EXTENSION}" CACHE STRING "" FORCE)
  cmake_language(DEFER CALL set CMAKE_CXX_OUTPUT_EXTENSION "${CMAKE_CXX_OUTPUT_EXTENSION}" CACHE STRING "" FORCE)
endif()

#
# QML Debugging
#
if (QT_ENABLE_QML_DEBUG)
  add_compile_definitions($<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)
endif()

#
# Package manager auto-setup
#
if (QT_CREATOR_ENABLE_PACKAGE_MANAGER_SETUP)
  include(${CMAKE_CURRENT_LIST_DIR}/package-manager.cmake)
endif()

#
# MaintenanceTool
#
if (QT_CREATOR_ENABLE_MAINTENANCE_TOOL_PROVIDER)
  function(qtc_maintenance_provider_missing_variable_message variable)
    message(STATUS "Qt Creator: ${variable} was not set. "
                  "Qt MaintenanceTool cannot be used to install missing Qt modules that you specify in find_package(). "
                  "To disable this message set QT_CREATOR_ENABLE_MAINTENANCE_TOOL_PROVIDER to OFF.")
  endfunction()

  if (NOT QT_QMAKE_EXECUTABLE)
    qtc_maintenance_provider_missing_variable_message(QT_QMAKE_EXECUTABLE)
    return()
  endif()

  if (CMAKE_VERSION GREATER_EQUAL "3.24")
    list(APPEND CMAKE_PROJECT_TOP_LEVEL_INCLUDES ${CMAKE_CURRENT_LIST_DIR}/maintenance_tool_provider.cmake)
  else()
    message(WARNING "Qt Creator: CMake version 3.24 is needed for MaintenanceTool find_package() provider. "
                    "To disable this warning set QT_CREATOR_ENABLE_MAINTENANCE_TOOL_PROVIDER to OFF.")
  endif()
endif()
