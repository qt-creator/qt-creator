add_qtc_test(tst_qtprofiler_recordingsession
  DEPENDS Utils
  DEFINES PROFILER_STATIC_LIBRARY
  INCLUDES ../../../../src/plugins/profiler ../../../../src/plugins
  SOURCES
    tst_recordingsession.cpp
    ../../../../src/plugins/profiler/sampler.cpp
    ../../../../src/plugins/profiler/samplerrecipe.cpp
    ../../../../src/plugins/profiler/sampler.h
)
