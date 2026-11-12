file(RELATIVE_PATH RELATIVE_TEST_PATH "${PROJECT_BINARY_DIR}" "${CMAKE_CURRENT_BINARY_DIR}")
file(RELATIVE_PATH TEST_RELATIVE_LIBEXEC_PATH "/${RELATIVE_TEST_PATH}" "/${IDE_LIBEXEC_PATH}")

# The Profiler plugin the tool loads links libdw. Its imported targets exist
# only in the directory that found it, so find it here too: that is what puts
# its DLLs on the test's PATH on Windows (see qtc_windows_runtime_path()).
find_package(elfutils QUIET)

add_qtc_test(tst_qtprofiler_cli
  CONDITION TARGET qtprofiler-cli
  DEFINES "TEST_RELATIVE_LIBEXEC_PATH=\"${TEST_RELATIVE_LIBEXEC_PATH}\""
  SOURCES tst_profilercli.cpp
)

# add_qtc_test creates the target only when the test is enabled.
if (TARGET tst_qtprofiler_cli)
  add_dependencies(tst_qtprofiler_cli qtprofiler-cli)
endif()

# Names the DLL a tool that cannot be loaded is missing.
if (WIN32 AND TARGET tst_qtprofiler_cli)
  target_link_libraries(tst_qtprofiler_cli PRIVATE imagehlp)
endif()
