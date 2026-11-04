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
        function(qtc_add_whisper)
            # Let option() take the values below. whisper.cpp asks for CMake 3.5.
            set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
            set(BUILD_SHARED_LIBS OFF)
            set(CMAKE_POSITION_INDEPENDENT_CODE ON)
            # Keep whisper.cpp and ggml private to the library that links them,
            # so they cannot clash with another copy of ggml in the process.
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
        qtc_add_whisper()
    endif()
endif()
