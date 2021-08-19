if(CMAKE_VERSION VERSION_LESS 3.17.0)
set(CMAKE_CURRENT_FUNCTION_LIST_DIR ${CMAKE_CURRENT_LIST_DIR})
endif()

# Enable separate debug information for the given target
# when QTC_SEPARATE_DEBUG_INFO is set
function(qtc_enable_separate_debug_info target installDestination)
    if (NOT QTC_SEPARATE_DEBUG_INFO)
        return()
    endif()
    if (NOT UNIX AND NOT MINGW)
        qtc_internal_install_pdb_files(${target} ${installDestination})
        return()
    endif()
    get_target_property(target_type ${target} TYPE)
    if (NOT target_type STREQUAL "MODULE_LIBRARY" AND
        NOT target_type STREQUAL "SHARED_LIBRARY" AND
        NOT target_type STREQUAL "EXECUTABLE")
        return()
    endif()
    get_property(target_source_dir TARGET ${target} PROPERTY SOURCE_DIR)
    get_property(skip_separate_debug_info DIRECTORY "${target_source_dir}" PROPERTY _qt_skip_separate_debug_info)
    if (skip_separate_debug_info)
        return()
    endif()

    unset(commands)
    if(APPLE)
        find_program(DSYMUTIL_PROGRAM dsymutil
          NO_PACKAGE_ROOT_PATH
          NO_CMAKE_PATH)
        set(copy_bin ${DSYMUTIL_PROGRAM})
        set(strip_bin ${CMAKE_STRIP})
        set(debug_info_suffix dSYM)
        set(copy_bin_out_arg --flat -o)
        set(strip_args -S)
    else()
        set(copy_bin ${CMAKE_OBJCOPY})
        set(strip_bin ${CMAKE_OBJCOPY})
        if(QNX)
            set(debug_info_suffix sym)
            set(debug_info_keep --keep-file-symbols)
            set(strip_args "--strip-debug -R.ident")
        else()
            set(debug_info_suffix debug)
            set(debug_info_keep --only-keep-debug)
            set(strip_args --strip-debug)
        endif()
    endif()
    if(APPLE)
        get_target_property(is_framework ${target} FRAMEWORK)
        if(is_framework)
            set(debug_info_bundle_dir "$<TARGET_BUNDLE_DIR:${target}>.${debug_info_suffix}")
            set(BUNDLE_ID Qt${target})
        else()
            set(debug_info_bundle_dir "$<TARGET_FILE:${target}>.${debug_info_suffix}")
            set(BUNDLE_ID ${target})
        endif()
        set(debug_info_contents_dir "${debug_info_bundle_dir}/Contents")
        set(debug_info_target_dir "${debug_info_contents_dir}/Resources/DWARF")
        configure_file(
            "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/QtcSeparateDebugInfo.Info.plist.in"
            "Info.dSYM.plist"
            )
        list(APPEND commands
            COMMAND ${CMAKE_COMMAND} -E make_directory ${debug_info_target_dir}
            COMMAND ${CMAKE_COMMAND} -E copy "Info.dSYM.plist" "${debug_info_contents_dir}/Info.plist"
            )
        set(debug_info_target "${debug_info_target_dir}/$<TARGET_FILE_BASE_NAME:${target}>")
        install(DIRECTORY ${debug_info_bundle_dir}
            DESTINATION ${installDestination}
            COMPONENT DebugInfo
            EXCLUDE_FROM_ALL
        )
    else()
        set(debug_info_target "$<TARGET_FILE_DIR:${target}>/$<TARGET_FILE_BASE_NAME:${target}>.${debug_info_suffix}")
        install(FILES ${debug_info_target}
            DESTINATION ${installDestination}
            COMPONENT DebugInfo
            EXCLUDE_FROM_ALL
        )
    endif()
    list(APPEND commands
        COMMAND ${copy_bin} ${debug_info_keep} $<TARGET_FILE:${target}>
                ${copy_bin_out_arg} ${debug_info_target}
        COMMAND ${strip_bin} ${strip_args} $<TARGET_FILE:${target}>
        )
    if(NOT APPLE)
        list(APPEND commands
            COMMAND ${CMAKE_OBJCOPY} --add-gnu-debuglink=${debug_info_target} $<TARGET_FILE:${target}>
            )
    endif()
    if(NOT CMAKE_HOST_WIN32)
        list(APPEND commands
            COMMAND chmod -x ${debug_info_target}
            )
    endif()
    add_custom_command(
        TARGET ${target}
        POST_BUILD
        ${commands}
        )
endfunction()

# Installs pdb files for given target into the specified install dir.
#
# MSVC generates 2 types of pdb files:
#  - compile-time generated pdb files (compile flag /Zi + /Fd<pdb_name>)
#  - link-time genereated pdb files (link flag /debug + /PDB:<pdb_name>)
#
# CMake allows changing the names of each of those pdb file types by setting
# the COMPILE_PDB_NAME_<CONFIG> and PDB_NAME_<CONFIG> properties. If they are
# left empty, CMake will compute the default names itself (or rather in certain cases
# leave it up to te compiler), without actually setting the property values.
#
# For installation purposes, CMake only provides a generator expression to the
# link time pdb file path, not the compile path one, which means we have to compute the
# path to the compile path pdb files ourselves.
# See https://gitlab.kitware.com/cmake/cmake/-/issues/18393 for details.
#
# For shared libraries and executables, we install the linker provided pdb file via the
# TARGET_PDB_FILE generator expression.
#
# For static libraries there is no linker invocation, so we need to install the compile
# time pdb file. We query the ARCHIVE_OUTPUT_DIRECTORY property of the target to get the
# path to the pdb file, and reconstruct the file name. We use a generator expression
# to append a possible debug suffix, in order to allow installation of all Release and
# Debug pdb files when using Ninja Multi-Config.
function(qtc_internal_install_pdb_files target install_dir_path)
    if(MSVC)
        get_target_property(target_type ${target} TYPE)

        if(target_type STREQUAL "SHARED_LIBRARY"
                OR target_type STREQUAL "EXECUTABLE"
                OR target_type STREQUAL "MODULE_LIBRARY")
            install(FILES "$<TARGET_PDB_FILE:${target}>"
                    DESTINATION "${install_dir_path}"
                    OPTIONAL
                    COMPONENT DebugInfo
                    EXCLUDE_FROM_ALL
            )

        elseif(target_type STREQUAL "STATIC_LIBRARY")
            get_target_property(lib_dir "${target}" ARCHIVE_OUTPUT_DIRECTORY)
            if(NOT lib_dir)
                message(FATAL_ERROR
                        "Can't install pdb file for static library ${target}. "
                        "The ARCHIVE_OUTPUT_DIRECTORY path is not known.")
            endif()
            set(pdb_name "${INSTALL_CMAKE_NAMESPACE}${target}$<$<CONFIG:Debug>:d>.pdb")
            set(compile_time_pdb_file_path "${lib_dir}/${pdb_name}")

            install(FILES "${compile_time_pdb_file_path}"
                    DESTINATION "${install_dir_path}"
                    OPTIONAL
                    COMPONENT DebugInfo
                    EXCLUDE_FROM_ALL
            )
        endif()
    endif()
endfunction()
