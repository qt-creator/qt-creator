# Generates the code of a tracepoint provider and attaches it to a target:
#
#     qt_add_tracepoints(<target> <provider file>)
#
# <provider file> is a .tracepoints file. The header generated from it is named
# after that file, <provider>_tracepoints_p.h, and is what the sources of the
# target include to reach the Q_TRACE macros the provider declares.
#
# Recording the trace needs a Qt that was configured for tracing, with
# "-trace <back end>". The back end decides the code that has to be generated
# here, and a Qt does not report the one it was built with, so it is named
# below rather than found. "ctf" is the one Qt Creator reads.

set(QT_TRACEPOINT_FORMAT "ctf" CACHE STRING
    "Trace back end that tracepoint code is generated for.")
set_property(CACHE QT_TRACEPOINT_FORMAT PROPERTY STRINGS ctf lttng etw)

# Qt installs the headers that the generated code includes only where it was
# configured for tracing, and tells a project using it nothing about whether it
# was. A Qt that was not gives a missing header in the middle of generated
# code, so the question is asked here, where the answer can be spelled out.
function(_qt_tracepoints_check_backend)
    if(NOT QT_TRACEPOINT_FORMAT STREQUAL "ctf")
        return()
    endif()

    # Asked once per configure, not once per project: check_cxx_source_compiles()
    # caches its answer, and no answer kept in the cache can be trusted, since
    # a Qt rebuilt in place with "-trace ctf" is not one that anything in the
    # cache tells from the Qt that was asked about. A project that does what the
    # message below says would otherwise be told the same thing again.
    get_property(asked GLOBAL PROPERTY _qt_tracepoints_ctf_asked)
    if(asked)
        return()
    endif()
    set_property(GLOBAL PROPERTY _qt_tracepoints_ctf_asked ON)

    include(CheckCXXSourceCompiles)
    unset(QT_TRACEPOINTS_HAVE_CTF CACHE)
    set(CMAKE_REQUIRED_LIBRARIES Qt6::CorePrivate)
    check_cxx_source_compiles(
        "#include <private/qctf_p.h>\nint main() { return 0; }"
        QT_TRACEPOINTS_HAVE_CTF)

    if(NOT QT_TRACEPOINTS_HAVE_CTF)
        message(FATAL_ERROR
            "Qt ${Qt6_VERSION} at ${QT6_INSTALL_PREFIX} was not configured for tracing "
            "in the Common Trace Format: it does not install private/qctf_p.h, which "
            "the generated tracepoint code includes. Build a Qt with "
            "\"configure -trace ctf\", or set QT_TRACEPOINT_FORMAT to the back end "
            "that the Qt you build against was configured with.")
    endif()
endfunction()

# Writes a file whose content is not already the one on disk, so that a file
# that did not change keeps its timestamp and what reads it is not compiled
# again.
function(_qt_tracepoints_write path content)
    if(EXISTS "${path}")
        file(READ "${path}" existing)
        if("${existing}" STREQUAL "${content}")
            return()
        endif()
    endif()
    file(WRITE "${path}" "${content}")
endfunction()

# Runs tracegen over the provider. The header is written beside itself and
# moved over the one that is there, so that a header that did not change keeps
# its timestamp and what includes it is not compiled again.
function(_qt_tracepoints_generate provider header)
    # find_package(Qt6 COMPONENTS Core) does not bring the tools package in, so
    # tracegen is not there to be asked about where it is in a build that has
    # no host Qt, and asking anyway ends the configuration with a message about
    # a non-existent target rather than about what is missing.
    if(NOT TARGET Qt6::tracegen)
        message(FATAL_ERROR
            "Qt ${Qt6_VERSION} at ${QT6_INSTALL_PREFIX} does not provide the tracegen tool, "
            "which generates the code of a tracepoint provider. Where the Qt you build "
            "against is a cross-build, point CMake at the tools of a host Qt of the same "
            "version with QT_HOST_PATH.")
    endif()

    get_target_property(tracegen Qt6::tracegen IMPORTED_LOCATION)
    if(NOT tracegen)
        get_target_property(configurations Qt6::tracegen IMPORTED_CONFIGURATIONS)
        if(configurations)
            list(GET configurations 0 configuration)
            get_target_property(tracegen Qt6::tracegen IMPORTED_LOCATION_${configuration})
        endif()
    endif()
    if(NOT tracegen)
        message(FATAL_ERROR "The imported target Qt6::tracegen names no executable.")
    endif()

    execute_process(
        COMMAND "${tracegen}" "${QT_TRACEPOINT_FORMAT}" "${provider}" "${header}.new"
        RESULT_VARIABLE result
        ERROR_VARIABLE output)
    if(NOT result EQUAL 0)
        file(REMOVE "${header}.new")
        message(FATAL_ERROR "Generating tracepoints from ${provider} failed.\n${output}")
    endif()

    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${header}.new" "${header}"
        RESULT_VARIABLE result
        ERROR_VARIABLE output)
    file(REMOVE "${header}.new")
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "Writing ${header} failed.\n${output}")
    endif()
endfunction()

function(qt_add_tracepoints target provider)
    find_package(Qt6 REQUIRED COMPONENTS Core CorePrivate)
    _qt_tracepoints_check_backend()

    get_filename_component(provider_path "${provider}" ABSOLUTE)
    get_filename_component(provider_name "${provider_path}" NAME_WLE)

    set(header_name "${provider_name}_tracepoints_p.h")
    set(header "${CMAKE_CURRENT_BINARY_DIR}/${header_name}")
    set(probes "${CMAKE_CURRENT_BINARY_DIR}/${provider_name}_tracepoints.cpp")

    # The header is written here rather than by a step of the build, so that
    # declaring a tracepoint is all it takes to have one. The provider is a
    # file the configuration depends on, so changing it has CMake run again --
    # a build does that of its own accord, and so does an IDE watching the
    # project -- and what reads the header then, the compiler and the code
    # model of the editor alike, sees the tracepoint that was just declared.
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${provider_path}")
    _qt_tracepoints_generate("${provider_path}" "${header}")

    # The probes of a provider have to be defined exactly once in the program.
    # The file is written while CMake runs rather than with file(GENERATE),
    # which leaves it to the generate step: a file that is not there yet takes
    # the GENERATED property to be named as a source, and that property is
    # scoped to the directory it was set in where CMP0118 is OLD, so a target
    # defined in another directory than the call could not name it.
    _qt_tracepoints_write("${probes}"
        "#define TRACEPOINT_CREATE_PROBES\n#define TRACEPOINT_DEFINE\n#include \"${header_name}\"\n")

    # The header is not named as a source of the target. Nothing has to build
    # it, and a generated header a target names is left out of the flags that
    # an IDE hands its code model, which leaves the model looking for the Qt
    # headers that the compiler has no trouble finding. What includes it finds
    # it through the include directory below, and reads it with the flags of
    # the file that included it.
    set_source_files_properties("${provider_path}" PROPERTIES HEADER_FILE_ONLY ON)
    target_sources(${target} PRIVATE "${provider_path}" "${probes}")
    target_include_directories(${target} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
    target_compile_definitions(${target} PRIVATE Q_TRACEPOINT)
    target_link_libraries(${target} PRIVATE Qt6::CorePrivate)

    if(QT_TRACEPOINT_FORMAT STREQUAL "lttng")
        find_package(LTTngUST REQUIRED)
        target_link_libraries(${target} PRIVATE LTTng::UST)
    endif()
endfunction()
