set(ALL_CMDBRIDGE_PLATFORMS "")

env_with_default("QTC_BUILD_EXECUTABLE_CMDBRIDGE" ENV_BUILD_EXECUTABLE_CMDBRIDGE ON)
option(BUILD_EXECUTABLE_CMDBRIDGE "Build the cmdbridge executable, required for fast container and remote device support" "${ENV_BUILD_EXECUTABLE_CMDBRIDGE}")
option(CMDBRIDGE_BUILD_VENDOR_MODE "Build the cmdbridge in vendor mode" OFF)

if(NOT BUILD_EXECUTABLE_CMDBRIDGE)
    add_feature_info("Executable CmdBridge" NO "")
    return()
endif()

function(go_build NAME SOURCES PLATFORMS ARCHITECTURES LDFLAGS)
    qtc_output_binary_dir(_output_binary_dir)
    set(OUTPUT_FOLDER "${_output_binary_dir}/${IDE_LIBEXEC_PATH}")
    foreach(ARCHITECTURE ${ARCHITECTURES})
        foreach(PLATFORM ${PLATFORMS})
            set(TARGET_NAME ${NAME}-${PLATFORM}-${ARCHITECTURE})
            set(OUTPUT ${OUTPUT_FOLDER}/${TARGET_NAME})

            if (${PLATFORM} STREQUAL "windows")
                string(APPEND OUTPUT ".exe")
            endif()

            set(VENDOR_MODE_OPTION "")
            if (CMDBRIDGE_BUILD_VENDOR_MODE)
                set(VENDOR_MODE_OPTION "-mod=vendor")
            endif()

            if ((${PLATFORM} STREQUAL "linux" OR (${PLATFORM} STREQUAL "windows" AND ${ARCHITECTURE} STREQUAL "amd64")) AND NOT UPX_BIN STREQUAL "UPX_BIN-NOTFOUND")
                add_custom_command(
                    OUTPUT "${OUTPUT}"
                    COMMAND ${CMAKE_COMMAND} -E env GOARCH=${ARCHITECTURE} GOOS=${PLATFORM} CGO_ENABLED=0 ${GO_BIN} build ${VENDOR_MODE_OPTION} -ldflags "-s -w ${LDFLAGS}" -o ${OUTPUT}
                    COMMAND ${UPX_BIN} -9 ${OUTPUT}
                    DEPENDS ${SOURCES}
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                    COMMENT "Building ${TARGET_NAME}"
                    VERBATIM

                )
            elseif(${PLATFORM} STREQUAL "linux" AND ${ARCHITECTURE} STREQUAL "arm")
                add_custom_command(
                    OUTPUT "${OUTPUT}"
                    COMMAND ${CMAKE_COMMAND} -E env GOARCH=${ARCHITECTURE} GOARM=5 GOOS=${PLATFORM} CGO_ENABLED=0 ${GO_BIN} build ${VENDOR_MODE_OPTION} -ldflags "-s -w ${LDFLAGS}" -o ${OUTPUT}
                    DEPENDS ${SOURCES}
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                    COMMENT "Building ${TARGET_NAME}"
                    VERBATIM
                )
            else()
                add_custom_command(
                    OUTPUT "${OUTPUT}"
                    COMMAND ${CMAKE_COMMAND} -E env GOARCH=${ARCHITECTURE} GOOS=${PLATFORM} CGO_ENABLED=0 ${GO_BIN} build ${VENDOR_MODE_OPTION} -ldflags "-s -w ${LDFLAGS}" -o ${OUTPUT}
                    DEPENDS ${SOURCES}
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                    COMMENT "Building ${TARGET_NAME}"
                    VERBATIM
                )
            endif()

            install(PROGRAMS ${OUTPUT} DESTINATION "${IDE_LIBEXEC_PATH}")

            list(APPEND ALL_CMDBRIDGE_PLATFORMS "${OUTPUT}")
        endforeach()
    endforeach()
    set(ALL_CMDBRIDGE_PLATFORMS ${ALL_CMDBRIDGE_PLATFORMS} PARENT_SCOPE)
endfunction()

find_program(GO_BIN go)
find_program(UPX_BIN upx)

if(NOT GO_BIN)
  message(FATAL_ERROR
      "Could not find a Go compiler, required for the CmdBridge.
For information about installing Go, see \"Installing the Go Compiler\" in the toplevel README.md file or https://code.qt.io/cgit/qt-creator/qt-creator.git/about/README.md#installing-the-go-compiler
You can also disable the CmdBridge by reconfiguring with -DBUILD_EXECUTABLE_CMDBRIDGE=OFF or setting the environment variable QTC_BUILD_EXECUTABLE_CMDBRIDGE=OFF
but note that disabling the CmdBridge leads to slow communication with containers and remote devices.")
endif()

set(CMDBRIDGEFEATURE_TEXT "with GO compiler at ${GO_BIN}")

set(SOURCE_FILES
    channelwriter.go
    cmdbridge.go
    exec.go
    fileaccess_windows.go
    fileaccess.go
    find.go
    is.go
    readfile.go
    stat.go
    writefile.go
    watcher.go
)

go_build(cmdbridge "${SOURCE_FILES}" "linux;darwin;windows" "amd64;arm64" "-X main.MagicPacketMarker=${GOBRIDGE_MAGIC_PACKET_MARKER}")
go_build(cmdbridge "${SOURCE_FILES}" "linux" "arm" "-X main.MagicPacketMarker=${GOBRIDGE_MAGIC_PACKET_MARKER}")

add_custom_target(CmdBridge
  DEPENDS
    ${ALL_CMDBRIDGE_PLATFORMS}
  SOURCES
    ${SOURCE_FILES}
)

add_feature_info("Executable CmdBridge" YES ${CMDBRIDGEFEATURE_TEXT})
