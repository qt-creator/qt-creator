# Self-signs a CmdBridge binary so that HarmonyOS will execute it. Run in script
# mode with SIGN_TOOL and BINARY set.
#
# HarmonyOS refuses to start an ELF unless it carries a .codesign section whose
# hashes match the file: without one execve fails with EACCES, with a stale one
# it fails with EPERM. The self-sign mode needs no certificate and no profile.

if(NOT EXISTS "${BINARY}")
    # Not every configuration builds the aarch64 binary, and a missing bridge is
    # not fatal: the client falls back to its slow file access.
    message(STATUS "CmdBridge: ${BINARY} was not built, nothing to sign")
    return()
endif()

execute_process(COMMAND "${SIGN_TOOL}" display-sign -inFile "${BINARY}"
    OUTPUT_VARIABLE sign_state ERROR_VARIABLE sign_state)

# Signing a binary that already has the section fails, so keep this idempotent.
if(NOT sign_state MATCHES "code signature is not found")
    message(STATUS "CmdBridge: ${BINARY} is already signed")
    return()
endif()

execute_process(COMMAND "${SIGN_TOOL}" sign -selfSign 1
        -inFile "${BINARY}" -outFile "${BINARY}.signed"
    RESULT_VARIABLE sign_result OUTPUT_VARIABLE sign_output ERROR_VARIABLE sign_output)

if(NOT sign_result EQUAL 0 OR NOT EXISTS "${BINARY}.signed")
    file(REMOVE "${BINARY}.signed")
    message(FATAL_ERROR "CmdBridge: failed to self-sign ${BINARY}:\n${sign_output}")
endif()

file(RENAME "${BINARY}.signed" "${BINARY}")
file(CHMOD "${BINARY}" PERMISSIONS
    OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

message(STATUS "CmdBridge: self-signed ${BINARY} for HarmonyOS")
