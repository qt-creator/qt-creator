set(ALL_CMDBRIDGE_PLATFORMS "")

function(go_build NAME SOURCES PLATFORMS ARCHITECTURES LDFLAGS)
    qtc_output_binary_dir(_output_binary_dir)
    set(OUTPUT_FOLDER "${_output_binary_dir}/${IDE_LIBEXEC_PATH}")
    foreach(ARCHITECTURE ${ARCHITECTURES})
        foreach(PLATFORM ${PLATFORMS})
            set(TARGET_NAME ${NAME}-${PLATFORM}-${ARCHITECTURE})
            set(OUTPUT ${OUTPUT_FOLDER}/${TARGET_NAME})

            if ((${PLATFORM} STREQUAL "linux" OR (${PLATFORM} STREQUAL "windows" AND ${ARCHITECTURE} STREQUAL "amd64")) AND NOT UPX_BIN STREQUAL "UPX_BIN-NOTFOUND")
                add_custom_command(
                    OUTPUT "${OUTPUT}"
                    COMMAND ${CMAKE_COMMAND} -E env GOARCH=${ARCHITECTURE} GOOS=${PLATFORM} go build -ldflags "-s -w ${LDFLAGS}" -o ${OUTPUT}
                    COMMAND ${UPX_BIN} -9 ${OUTPUT}
                    DEPENDS ${SOURCES}
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                    COMMENT "Building ${TARGET_NAME}"
                    VERBATIM

                )
            else()
                add_custom_command(
                    OUTPUT "${OUTPUT}"
                    COMMAND ${CMAKE_COMMAND} -E env GOARCH=${ARCHITECTURE} GOOS=${PLATFORM} go build -ldflags "-s -w ${LDFLAGS}" -o ${OUTPUT}
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

if (${GO_BIN} STREQUAL "GO_BIN-NOTFOUND")
    message(WARNING "No GO compiler found, cmdbridge will not be built")
else ()
    message(STATUS "Found go compiler at: ${GO_BIN}")

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

    add_custom_target(CmdBridge
      DEPENDS
        ${ALL_CMDBRIDGE_PLATFORMS})
endif()
