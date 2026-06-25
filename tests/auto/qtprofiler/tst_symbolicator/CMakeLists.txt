# The symbolicator only exists on macOS (it drives mach + CoreSymbolication).
if (NOT APPLE)
  return()
endif()

# A dylib the test dlopen()s at runtime to exercise symbolication of a library
# loaded after a session starts. Built as a loadable module and never linked into
# the test, so it is absent until the test loads it.
add_library(tst_qtprofiler_symbolicator_fixture MODULE fixture.cpp)
set_target_properties(tst_qtprofiler_symbolicator_fixture PROPERTIES
  PREFIX "lib"
  SUFFIX ".dylib"
  OUTPUT_NAME "symbolicatorfixture"
)

add_qtc_test(tst_qtprofiler_symbolicator
  DEPENDS Utils
  DEFINES QMLPROFILER_STATIC_LIBRARY
  INCLUDES ../../../../src/plugins/profiler
  SOURCES
    tst_symbolicator.cpp
    ../../../../src/plugins/profiler/symbolicator.cpp
    ../../../../src/plugins/profiler/symbolicator.h
)

# add_qtc_test creates the target only when the test is enabled.
if (TARGET tst_qtprofiler_symbolicator)
  add_dependencies(tst_qtprofiler_symbolicator tst_qtprofiler_symbolicator_fixture)
  target_compile_definitions(tst_qtprofiler_symbolicator PRIVATE
    SYMBOLICATOR_FIXTURE_DYLIB="$<TARGET_FILE:tst_qtprofiler_symbolicator_fixture>")
endif()
