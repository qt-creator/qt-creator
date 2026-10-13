# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Points the build tools a Qt package on the device names at the copies of qtchost in Qt
# Creator's package. The device refuses to execute the tools in the Qt's own tree, and the
# copies map them instead. Qt Creator sets QTC_QT_HOST_LIBEXECS for them to find the tools.
# AUTOMOC and friends as well as Qt's own custom commands take the location of the imported
# target, and a dependency provider is the one place that sees each target as it appears.

set(_qtc_tool_wrappers "/data/app/qtctools.org/qtctools_1.0/bin")

# A Qt built for HarmonyOS imports its tools as modules, whose names Qt's custom commands then
# run literally. Declaring them as executables first makes its Targets file skip them.
macro(qtc_declare_module_tools name)
    unset(_qtc_module)
    unset(_qtc_targets)
    if(QT_HOST_PATH_CMAKE_DIR)
        set(_qtc_targets_file "${QT_HOST_PATH_CMAKE_DIR}/${name}/${name}Targets.cmake")
    else()
        set(_qtc_targets_file "${Qt6_DIR}/../${name}/${name}Targets.cmake")
    endif()
    if(EXISTS "${_qtc_targets_file}")
        file(STRINGS "${_qtc_targets_file}" _qtc_module REGEX "MODULE IMPORTED")
        file(STRINGS "${_qtc_targets_file}" _qtc_targets
             REGEX "^foreach\\(_cmake_expected_target IN ITEMS ")
    endif()
    if(_qtc_module AND _qtc_targets)
        string(REGEX REPLACE "^foreach\\(_cmake_expected_target IN ITEMS (.*)\\)$" "\\1"
               _qtc_targets "${_qtc_targets}")
        separate_arguments(_qtc_targets)
        get_filename_component(_qtc_libdir "${_qtc_targets_file}/../../.." ABSOLUTE)
        foreach(_qtc_target IN LISTS _qtc_targets)
            if(NOT TARGET ${_qtc_target})
                string(REPLACE "Qt6::" "" _qtc_tool "${_qtc_target}")
                add_executable(${_qtc_target} IMPORTED)
                set_target_properties(${_qtc_target} PROPERTIES
                    IMPORTED_LOCATION "${_qtc_libdir}/lib${_qtc_tool}.so")
            endif()
        endforeach()
    endif()
endmacro()

macro(qtc_provide_dependency method name)
    if("${name}" MATCHES "^Qt6.+Tools$")
        qtc_declare_module_tools(${name})
    endif()
    find_package(${name} ${ARGN} BYPASS_PROVIDER)
    foreach(_qtc_tool IN ITEMS moc rcc uic cmake_automoc_parser qlalr)
        if(TARGET Qt6::${_qtc_tool} AND EXISTS "${_qtc_tool_wrappers}/${_qtc_tool}")
            get_target_property(_qtc_imported Qt6::${_qtc_tool} IMPORTED)
            if(_qtc_imported)
                get_target_property(_qtc_configs Qt6::${_qtc_tool} IMPORTED_CONFIGURATIONS)
                foreach(_qtc_config IN LISTS _qtc_configs)
                    set_target_properties(Qt6::${_qtc_tool} PROPERTIES
                        IMPORTED_LOCATION_${_qtc_config} "${_qtc_tool_wrappers}/${_qtc_tool}")
                endforeach()
                set_target_properties(Qt6::${_qtc_tool} PROPERTIES
                    IMPORTED_LOCATION "${_qtc_tool_wrappers}/${_qtc_tool}")
            endif()
        endif()
    endforeach()
endmacro()

cmake_language(SET_DEPENDENCY_PROVIDER qtc_provide_dependency SUPPORTED_METHODS FIND_PACKAGE)
