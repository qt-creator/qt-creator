add_subdirectory(cplusplus)
add_subdirectory(quickjs)
add_subdirectory(syntax-highlighting)
add_subdirectory(libvterm)
if(NOT EMSCRIPTEN)
    # libptyqt has no WebAssembly backend (no pseudo-terminals under Emscripten).
    add_subdirectory(libptyqt)
endif()
add_subdirectory(qtkeychain)
add_subdirectory(lua)
add_subdirectory(sol2)

if(WIN32)
    add_subdirectory(winpty)
endif()

if(EMSCRIPTEN)
    # libarchive relies on POSIX disk-walking APIs unavailable under Emscripten.
    # Unlike libptyqt this keeps a stub INTERFACE target, because dependents outside Utils
    # name it unconditionally (the unarchiver autotest), and a missing target would silently
    # turn into a plain -lQtCLibArchive link flag.
    add_library(QtCLibArchive INTERFACE)
else()
    find_package(LibArchive QUIET)

    if(LibArchive_FOUND)
      set(QTC_USE_SYSTEM_LIBARCHIVE_DEFAULT ON)
    else()
      set(QTC_USE_SYSTEM_LIBARCHIVE_DEFAULT OFF)
    endif()
    option(QTC_USE_SYSTEM_LIBARCHIVE "Use the system provided libarchive" ${QTC_USE_SYSTEM_LIBARCHIVE_DEFAULT})

    if (NOT QTC_USE_SYSTEM_LIBARCHIVE)
        add_subdirectory(xz)
        add_subdirectory(bzip2)
        add_subdirectory(zlib)

        set(ENABLE_WERROR OFF)
        set(ENABLE_TEST OFF)
        set(ENABLE_INSTALL OFF)
        set(BUILD_SHARED_LIBS OFF)

        add_subdirectory(libarchive)
        add_library(QtCLibArchive ALIAS archive_static)

        IF (NOT MSVC)
          # Disable warnings
          target_compile_options(archive_static PRIVATE "-w")
        ENDIF()
    else()
        # make IMPORTED_GLOBAL to be able to create ALIAS target
        set_property(TARGET LibArchive::LibArchive PROPERTY IMPORTED_GLOBAL TRUE)
        add_library(QtCLibArchive ALIAS LibArchive::LibArchive)
    endif()

    add_feature_info("Use system libarchive" QTC_USE_SYSTEM_LIBARCHIVE "")
endif()

# whisper.cpp transcribes speech for voice input.
if(NOT EMSCRIPTEN AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/whisper.cpp/CMakeLists.txt)
    option(BUILD_LIBRARY_WHISPER "Build whisper.cpp for voice input."
           ${BUILD_LIBRARIES_BY_DEFAULT})
    if(BUILD_LIBRARY_WHISPER)
        # Metal covers macOS. Elsewhere, Vulkan reaches the GPUs of all vendors.
        if(WIN32 OR (UNIX AND NOT APPLE))
            option(BUILD_WHISPER_VULKAN
                   "Let voice input transcribe on the GPU with Vulkan, if the Vulkan SDK is found."
                   ON)
        endif()
        # The oldest Vulkan headers that ggml's Vulkan backend is known to build with.
        set(QTC_WHISPER_VULKAN_MIN_VERSION 1.3.275)
        function(qtc_whisper_find_vulkan resultVar)
            set(${resultVar} OFF PARENT_SCOPE)
            if(NOT BUILD_WHISPER_VULKAN)
                return()
            endif()
            if(DEFINED ENV{VULKAN_SDK})
                list(APPEND CMAKE_PREFIX_PATH "$ENV{VULKAN_SDK}")
            endif()
            find_package(Vulkan COMPONENTS glslc QUIET)
            find_package(SPIRV-Headers CONFIG QUIET)
            set(missing "")
            if(NOT Vulkan_FOUND)
                list(APPEND missing "the Vulkan headers and loader")
            elseif(NOT Vulkan_VERSION OR Vulkan_VERSION VERSION_LESS QTC_WHISPER_VULKAN_MIN_VERSION)
                list(APPEND missing "Vulkan ${QTC_WHISPER_VULKAN_MIN_VERSION} or newer (found ${Vulkan_VERSION})")
            endif()
            if(NOT Vulkan_GLSLC_EXECUTABLE)
                list(APPEND missing "glslc")
            endif()
            if(NOT SPIRV-Headers_FOUND)
                list(APPEND missing "the SPIRV-Headers CMake package")
            endif()
            if(missing)
                set(found OFF)
                list(JOIN missing ", " missing)
                message(STATUS "Voice input transcribes on the CPU only. The GPU needs ${missing}.")
            else()
                set(found ON)
            endif()
            add_feature_info("Voice input on the GPU with Vulkan" found "")
            set(${resultVar} ${found} PARENT_SCOPE)
        endfunction()
        function(qtc_add_whisper)
            qtc_whisper_find_vulkan(use_vulkan)
            # Let option() take the values below. whisper.cpp asks for CMake 3.5.
            set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
            # The Vulkan backend links the Vulkan loader, which not every system
            # has. As a module, ggml loads it only where it can be loaded, which
            # needs ggml and whisper.cpp as shared libraries.
            set(BUILD_SHARED_LIBS ${use_vulkan})
            set(GGML_BACKEND_DL ${use_vulkan})
            set(GGML_VULKAN ${use_vulkan})
            set(CMAKE_POSITION_INDEPENDENT_CODE ON)
            # Keep whisper.cpp and ggml private to the library that links them,
            # or to their own shared libraries, so they cannot clash with
            # another copy of ggml in the process.
            set(CMAKE_C_VISIBILITY_PRESET hidden)
            set(CMAKE_CXX_VISIBILITY_PRESET hidden)
            set(CMAKE_OBJC_VISIBILITY_PRESET hidden)
            set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)
            set(WHISPER_BUILD_TESTS OFF)
            set(WHISPER_BUILD_EXAMPLES OFF)
            set(WHISPER_BUILD_SERVER OFF)
            # Neither a runtime that the package would have to ship, nor code
            # for the CPU of the build machine.
            set(GGML_OPENMP OFF)
            set(GGML_NATIVE OFF)
            # ggml would set a global RULE_LAUNCH_COMPILE to any ccache or sccache
            # it finds, for all of Qt Creator, which breaks /Zi builds with MSVC.
            set(GGML_CCACHE OFF)
            # MSVC cannot target ARM features by flag, so ggml only accepts it
            # with GGML_NATIVE, which runs probes on the build machine and
            # defines __ARM_FEATURE_* from their results. Answer the probes
            # here instead, with what every ARM64 Windows device has.
            if(CMAKE_C_COMPILER_ID STREQUAL "MSVC"
               AND CMAKE_C_COMPILER_ARCHITECTURE_ID STREQUAL "ARM64")
                set(GGML_NATIVE ON)
                set(GGML_MACHINE_SUPPORTS_fma ON)
                foreach(feature dotprod i8mm sve sme)
                    set(GGML_MACHINE_SUPPORTS_${feature} OFF)
                    set(GGML_MACHINE_SUPPORTS_no${feature} OFF)
                endforeach()
            endif()
            # x86 needs Haswell or newer. Set explicitly, because ggml's defaults
            # differ between builds with and without SOURCE_DATE_EPOCH.
            foreach(feature SSE42 AVX AVX2 FMA F16C BMI2)
                set(GGML_${feature} ON)
            endforeach()
            # The BLAS backend calls Accelerate functions that macOS 13.3 introduced.
            if(APPLE AND CMAKE_OSX_DEPLOYMENT_TARGET
               AND CMAKE_OSX_DEPLOYMENT_TARGET VERSION_LESS 13.3)
                set(GGML_BLAS OFF)
            endif()
            # Only what voice input links is built, and whisper.cpp's install
            # rules stay out of Qt Creator's.
            add_subdirectory(whisper.cpp EXCLUDE_FROM_ALL)
        endfunction()
        # Elsewhere than with MSVC, separate debug info is extracted by a
        # POST_BUILD command, which CMake only accepts in the directory that
        # created the target. whisper.cpp's directories are done by now.
        function(qtc_whisper_separate_debug_info target destination)
            if(NOT UNIX AND NOT MINGW)
                qtc_enable_separate_debug_info(${target} "${destination}")
            endif()
        endfunction()
        # Puts the shared libraries where Qt Creator's are, and the backend
        # modules where VoiceInput looks for them. Called by VoiceInput, because
        # whisper.cpp is only built for it, and installing what is not built
        # fails.
        function(qtc_install_whisper)
            get_target_property(type whisper TYPE)
            if(NOT type STREQUAL "SHARED_LIBRARY")
                return()
            endif()
            qtc_output_binary_dir(output_binary_dir)
            if(WIN32)
                set(backend_path "${IDE_BIN_PATH}")
            else()
                set(backend_path "${IDE_LIBRARY_PATH}")
            endif()
            foreach(target whisper ggml ggml-base)
                # The export must not point into the source tree, and nothing
                # outside it compiles against these headers.
                get_target_property(includes ${target} INTERFACE_INCLUDE_DIRECTORIES)
                if(includes)
                    set_target_properties(${target} PROPERTIES
                        INTERFACE_INCLUDE_DIRECTORIES "$<BUILD_INTERFACE:${includes}>")
                endif()
                # Another ggml in the process must not stand in for this one,
                # which a matching file name or soname would make it do.
                set_target_properties(${target} PROPERTIES
                    OUTPUT_NAME "qtc-${target}"
                    PUBLIC_HEADER ""
                    RUNTIME_OUTPUT_DIRECTORY "${output_binary_dir}/${IDE_BIN_PATH}"
                    LIBRARY_OUTPUT_DIRECTORY "${output_binary_dir}/${IDE_LIBRARY_PATH}"
                    INSTALL_RPATH "\$ORIGIN"
                )
                # VoiceInput's export names the shared libraries it links.
                install(TARGETS ${target}
                    EXPORT QtCreator
                    RUNTIME
                        DESTINATION "${IDE_BIN_PATH}"
                        OPTIONAL
                    LIBRARY
                        DESTINATION "${IDE_LIBRARY_PATH}"
                        NAMELINK_SKIP
                        OPTIONAL
                    ARCHIVE
                        DESTINATION "${IDE_DEVEL_LIBRARY_ARCHIVE_INSTALL_PATH}"
                        COMPONENT Devel EXCLUDE_FROM_ALL
                        OPTIONAL
                )
                qtc_whisper_separate_debug_info(${target} "${IDE_LIBRARY_PATH}")
            endforeach()
            foreach(backend IN LISTS GGML_AVAILABLE_BACKENDS)
                if(NOT TARGET ${backend})
                    continue()
                endif()
                set_target_properties(${backend} PROPERTIES
                    LIBRARY_OUTPUT_DIRECTORY "${output_binary_dir}/${backend_path}"
                    INSTALL_RPATH "\$ORIGIN"
                )
                install(TARGETS ${backend} LIBRARY DESTINATION "${backend_path}")
                qtc_whisper_separate_debug_info(${backend} "${backend_path}")
            endforeach()
            file(RELATIVE_PATH relative_backend_path "/${IDE_BIN_PATH}" "/${backend_path}")
            target_compile_definitions(whisper INTERFACE
                QTC_GGML_BACKEND_PATH="${relative_backend_path}")
        endfunction()
        qtc_add_whisper()
    endif()
endif()
