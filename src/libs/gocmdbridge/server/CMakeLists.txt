# Selects which CmdBridge server implementation to build. The two live side by
# side in go/ and c/, each with its own CMakeLists.txt; only one is added.

env_with_default("QTC_BUILD_EXECUTABLE_CMDBRIDGE" ENV_BUILD_EXECUTABLE_CMDBRIDGE ON)
option(BUILD_EXECUTABLE_CMDBRIDGE "Build the cmdbridge executable, required for fast container and remote device support" "${ENV_BUILD_EXECUTABLE_CMDBRIDGE}")
option(CMDBRIDGE_BUILD_VENDOR_MODE "Build the cmdbridge in vendor mode" OFF)
option(BUILD_CMDBRIDGE_C "Build the C implementation of CmdBridge instead of the Go one" OFF)

if(NOT BUILD_EXECUTABLE_CMDBRIDGE)
    add_feature_info("Executable CmdBridge" NO "")
    return()
endif()

# Both implementations install their binaries here, named cmdbridge-<os>-<arch>,
# which is how the client looks them up.
qtc_output_binary_dir(_output_binary_dir)
set(CMDBRIDGE_OUTPUT_FOLDER "${_output_binary_dir}/${IDE_LIBEXEC_PATH}")

# Each subdirectory defines the CmdBridge target and sets CMDBRIDGE_FEATURE_TEXT
# to describe how it was built.
if(BUILD_CMDBRIDGE_C)
    add_subdirectory(c)
else()
    add_subdirectory(go)
endif()

add_feature_info("Executable CmdBridge" YES "${CMDBRIDGE_FEATURE_TEXT}")
