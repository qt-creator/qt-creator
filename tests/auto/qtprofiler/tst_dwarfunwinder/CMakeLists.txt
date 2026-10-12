# The unwinder needs libdw.
find_package(elfutils)
if (NOT elfutils_FOUND)
  return()
endif()

add_qtc_test(tst_qtprofiler_dwarfunwinder
  DEPENDS Utils elfutils::dw elfutils::elf
  DEFINES PROFILER_STATIC_LIBRARY WITH_LIBDW
  INCLUDES ../../../../src/plugins/profiler
  SOURCES
    tst_dwarfunwinder.cpp
    ../../../../src/plugins/profiler/perfdwarfunwinder.cpp
    ../../../../src/plugins/profiler/perfdwarfunwinder.h
    ../../../../src/plugins/profiler/perfelfutils.h
    ../../../../src/plugins/profiler/perfregisters.h
)

# add_qtc_test creates the target only when the test is enabled.
if (TARGET tst_qtprofiler_dwarfunwinder)
  target_compile_definitions(tst_qtprofiler_dwarfunwinder PRIVATE
    FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixture")
  if (NOT WIN32)
    # libdw loads the backend of an architecture by its file name, from where
    # an installation puts it next to libdw (see scripts/deploy.py).
    set_property(TEST tst_qtprofiler_dwarfunwinder APPEND PROPERTY
      ENVIRONMENT "LD_LIBRARY_PATH=$<TARGET_FILE_DIR:elfutils::elf>/elfutils")
  endif()
endif()
