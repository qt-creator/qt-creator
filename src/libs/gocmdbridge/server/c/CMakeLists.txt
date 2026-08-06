# The C implementation of the CmdBridge server. Only cmdbridge.c is compiled;
# it #includes the rest, so the whole server is one translation unit.
#
# By default it is built for the host with the project's C compiler. With
# CMDBRIDGE_USE_ZIG=ON, zig cc cross-compiles it for every platform the Go
# implementation supports.

option(CMDBRIDGE_USE_ZIG "Use zig cc for cross-compilation to all Go-supported platforms" OFF)

find_package(Threads REQUIRED)

# Everything cmdbridge.c pulls in, directly or through another file. This is the
# dependency list of the build command, so a file missing here means edits to it
# do not trigger a rebuild.
set(CMDBRIDGE_C_SOURCES
    cmdbridge.c
    containers.c
    cbor.c
    fileaccess.c
    fileaccess_posix.c
    fileaccess_win.c
    cancel.c
    wire.c
    environment.c
    stat.c
    fileops.c
    is.c
    readfile.c
    writefile.c
    find.c
    find_posix.c
    find_win.c
    exec.c
    exec_posix.c
    exec_win.c
    watcher.c
    watcher_linux.c
    watcher_apple.c
    watcher_win.c
    watcher_none.c
    socketforward.c
)

# Maps the CMake host system to the cmdbridge-<os>-<arch> naming the client
# uses to find the binary.
function(cmdbridge_detect_host_platform OUT_PLATFORM OUT_ARCH)
    string(TOLOWER "${CMAKE_SYSTEM_NAME}" _cmdb_platform)
    if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        set(_cmdb_platform "darwin")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        set(_cmdb_platform "windows")
    endif()

    set(_cmdb_arch "${CMAKE_SYSTEM_PROCESSOR}")
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64|i[3-6]86)$")
        set(_cmdb_arch "amd64")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(ARM64|aarch64|armv8b|armv8l)$")
        set(_cmdb_arch "arm64")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^arm$")
        set(_cmdb_arch "arm")
    endif()

    set("${OUT_PLATFORM}" "${_cmdb_platform}" PARENT_SCOPE)
    set("${OUT_ARCH}" "${_cmdb_arch}" PARENT_SCOPE)
endfunction()

# Registers the protocol test and the CBOR fuzz target against `BINARY`.
function(cmdbridge_add_tests BINARY)
    find_package(Python3 3.8 COMPONENTS Interpreter)
    if(NOT Python3_Interpreter_FOUND)
        message(WARNING "python3 not found, skipping C CmdBridge tests")
        return()
    endif()

    add_test(
        NAME tst_cmdbridge_c
        COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/cmdbridge_py_test.py"
            "${BINARY}"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )

    # The CBOR fuzz target #includes cbor.c, so it cannot drift away from the
    # decoder that ships. Run with no input it executes its structural
    # self-tests, which is cheap and keeps the target compiling. Needs a POSIX
    # libc (unistd.h), so it is skipped with MSVC; use zig or clang to fuzz on
    # Windows.
    if(NOT MSVC)
        add_executable(tst_cmdbridge_cbor_fuzz
            "${CMAKE_CURRENT_SOURCE_DIR}/cmdbridge_fuzz.c")
        target_compile_definitions(tst_cmdbridge_cbor_fuzz PRIVATE _GNU_SOURCE)
        set_target_properties(tst_cmdbridge_cbor_fuzz PROPERTIES
            C_STANDARD 11
            C_STANDARD_REQUIRED ON)
    endif()
endfunction()

if(CMDBRIDGE_USE_ZIG)
    find_program(ZIG_CC zig)
    if(NOT ZIG_CC)
        message(FATAL_ERROR
            "zig not found. Install zig (https://ziglang.org/download/) or use:\n"
            "  brew install zig (macOS)\n"
            "  apt install zig (Debian/Ubuntu via PPA)\n"
            "Zig is required for cross-compiling the C CmdBridge to all Go-supported platforms.")
    endif()

    # Size-optimized and stripped (97K vs 1.8M debug). The language and feature
    # flags must match the host build below: the sources are C11
    # (_Thread_local) and need _GNU_SOURCE on the Linux targets.
    set(ZIG_FLAGS -std=c11 -D_GNU_SOURCE -Wall -Wextra -Os)
    set(ZIG_DEFS -DGOBRIDGE_MAGIC_PACKET_MARKER="${GOBRIDGE_MAGIC_PACKET_MARKER}")

    # Builds one target triple.
    function(zig_build_target NAME PLATFORM ARCH TARGET_TRIPLE OUTPUT)
        set(ZIG_LINKER_FLAGS "-Wl,-s")
        if(PLATFORM STREQUAL "windows")
            # ws2_32 for AF_UNIX sockets, bcrypt for BCryptGenRandom
            # (temporary file names).
            list(APPEND ZIG_LINKER_FLAGS "-Xlinker" "/subsystem:console"
                "-lws2_32" "-lbcrypt")
        endif()
        add_custom_command(
            OUTPUT "${OUTPUT}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMDBRIDGE_OUTPUT_FOLDER}"
            COMMAND ${ZIG_CC} cc ${ZIG_FLAGS} -target ${TARGET_TRIPLE} ${ZIG_DEFS}
                ${CMAKE_CURRENT_SOURCE_DIR}/cmdbridge.c
                -o "${OUTPUT}" ${ZIG_LINKER_FLAGS}
            DEPENDS ${CMDBRIDGE_C_SOURCES}
            COMMENT "Building ${NAME}-${PLATFORM}-${ARCH} (zig target: ${TARGET_TRIPLE})"
            VERBATIM
        )

        install(PROGRAMS ${OUTPUT} DESTINATION "${IDE_LIBEXEC_PATH}")

        add_custom_target("${NAME}-${PLATFORM}-${ARCH}" DEPENDS "${OUTPUT}")
        list(APPEND ALL_CMDBRIDGE_PLATFORMS "${NAME}-${PLATFORM}-${ARCH}")
        set(ALL_CMDBRIDGE_PLATFORMS ${ALL_CMDBRIDGE_PLATFORMS} PARENT_SCOPE)
    endfunction()

    set(ALL_CMDBRIDGE_PLATFORMS "")

    # Linux uses musl, so the result is static and runs in any container.
    zig_build_target(cmdbridge linux amd64 "x86_64-linux-musl"
        "${CMDBRIDGE_OUTPUT_FOLDER}/cmdbridge-linux-amd64")
    zig_build_target(cmdbridge linux arm64 "aarch64-linux-musl"
        "${CMDBRIDGE_OUTPUT_FOLDER}/cmdbridge-linux-arm64")
    zig_build_target(cmdbridge linux arm "arm-linux-musleabihf"
        "${CMDBRIDGE_OUTPUT_FOLDER}/cmdbridge-linux-arm")

    zig_build_target(cmdbridge darwin amd64 "x86_64-macos"
        "${CMDBRIDGE_OUTPUT_FOLDER}/cmdbridge-darwin-amd64")
    zig_build_target(cmdbridge darwin arm64 "aarch64-macos"
        "${CMDBRIDGE_OUTPUT_FOLDER}/cmdbridge-darwin-arm64")

    zig_build_target(cmdbridge windows amd64 "x86_64-windows"
        "${CMDBRIDGE_OUTPUT_FOLDER}/cmdbridge-windows-amd64.exe")
    zig_build_target(cmdbridge windows arm64 "aarch64-windows"
        "${CMDBRIDGE_OUTPUT_FOLDER}/cmdbridge-windows-arm64.exe")

    # The BSD targets need zig 0.16 or newer; earlier versions ship no BSD
    # libc and fail with "'stdio.h' file not found". Skip them instead of
    # breaking the build, because a missing binary is not fatal: the client
    # falls back to its slow file access when it cannot find or start one.
    # That is also the safety net for OpenBSD and NetBSD in general, which
    # are cross-compiled but have not been run on the real thing.
    execute_process(COMMAND ${ZIG_CC} version
        OUTPUT_VARIABLE ZIG_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)
    if(ZIG_VERSION VERSION_GREATER_EQUAL "0.16")
        zig_build_target(cmdbridge freebsd amd64 "x86_64-freebsd"
            "${CMDBRIDGE_OUTPUT_FOLDER}/cmdbridge-freebsd-amd64")
        zig_build_target(cmdbridge freebsd arm64 "aarch64-freebsd"
            "${CMDBRIDGE_OUTPUT_FOLDER}/cmdbridge-freebsd-arm64")

        zig_build_target(cmdbridge openbsd amd64 "x86_64-openbsd"
            "${CMDBRIDGE_OUTPUT_FOLDER}/cmdbridge-openbsd-amd64")
        zig_build_target(cmdbridge openbsd arm64 "aarch64-openbsd"
            "${CMDBRIDGE_OUTPUT_FOLDER}/cmdbridge-openbsd-arm64")

        zig_build_target(cmdbridge netbsd amd64 "x86_64-netbsd"
            "${CMDBRIDGE_OUTPUT_FOLDER}/cmdbridge-netbsd-amd64")
        zig_build_target(cmdbridge netbsd arm64 "aarch64-netbsd"
            "${CMDBRIDGE_OUTPUT_FOLDER}/cmdbridge-netbsd-arm64")
    else()
        message(STATUS
            "CmdBridge: zig ${ZIG_VERSION} ships no BSD libc, skipping the "
            "FreeBSD, OpenBSD and NetBSD targets")
    endif()

    add_custom_target(CmdBridge DEPENDS ${ALL_CMDBRIDGE_PLATFORMS}
        SOURCES ${CMDBRIDGE_C_SOURCES})

    set(CMDBRIDGE_FEATURE_TEXT "with zig cc (musl for Linux)" PARENT_SCOPE)

    if(WITH_TESTS)
        cmdbridge_detect_host_platform(C_TEST_PLATFORM C_TEST_ARCH)
        set(C_TEST_TARGET
            "${CMDBRIDGE_OUTPUT_FOLDER}/cmdbridge-${C_TEST_PLATFORM}-${C_TEST_ARCH}")
        if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
            set(C_TEST_TARGET "${C_TEST_TARGET}.exe")
        endif()
        cmdbridge_add_tests("${C_TEST_TARGET}")
    endif()
else()
    cmdbridge_detect_host_platform(C_PLATFORM C_ARCH)
    set(C_TARGET "${CMDBRIDGE_OUTPUT_FOLDER}/cmdbridge-${C_PLATFORM}-${C_ARCH}")
    if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        set(C_TARGET "${C_TARGET}.exe")
    endif()

    set(C_FLAGS -D_GNU_SOURCE -std=c11 -Wall -Wextra -O2)
    if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        # ws2_32 for AF_UNIX sockets, bcrypt for BCryptGenRandom.
        list(APPEND C_FLAGS -lws2_32 -lbcrypt)
    else()
        list(APPEND C_FLAGS -pthread)
    endif()

    add_custom_command(
        OUTPUT "${C_TARGET}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMDBRIDGE_OUTPUT_FOLDER}"
        COMMAND ${CMAKE_C_COMPILER} ${C_FLAGS}
            -DGOBRIDGE_MAGIC_PACKET_MARKER="${GOBRIDGE_MAGIC_PACKET_MARKER}"
            ${CMAKE_CURRENT_SOURCE_DIR}/cmdbridge.c
            -o "${C_TARGET}"
        DEPENDS ${CMDBRIDGE_C_SOURCES}
        COMMENT "Building C CmdBridge (cmdbridge-${C_PLATFORM}-${C_ARCH})"
        VERBATIM
    )

    add_custom_target(CmdBridge DEPENDS "${C_TARGET}"
        SOURCES ${CMDBRIDGE_C_SOURCES})

    install(PROGRAMS ${C_TARGET} DESTINATION "${IDE_LIBEXEC_PATH}")

    set(CMDBRIDGE_FEATURE_TEXT "with C compiler" PARENT_SCOPE)

    if(WITH_TESTS)
        cmdbridge_add_tests("${C_TARGET}")
    endif()
endif()
