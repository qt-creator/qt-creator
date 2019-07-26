#.rst:
# FindGoogleBenchmark
# -----------------
#
# Try to locate the GoogleBenchmark source files, and then build them as a
# static library.
#
# The ``GOOGLEBENCHMARK_DIR`` (CMake or Environment) variable should be used
# to pinpoint the GoogleBenchmark source files.
#
# If found, this will define the following variables:
#
# ``GoogleBenchmark_FOUND``
#     True if the GoogleBenchmark source package has been found.
#
# ``GoogleBenchmark``
#     Target compiled as static library.
#

find_path(GOOGLE_BENCHMARK_INCLUDE_DIR
  NAMES benchmark/benchmark.h
  PATH_SUFFIXES include
  HINTS
    "${GOOGLEBENCHMARK_DIR}" ENV GOOGLEBENCHMARK_DIR
    "${CMAKE_SOURCE_DIR}/benchmark"
    "${CMAKE_SOURCE_DIR}/../benchmark"
    "${CMAKE_SOURCE_DIR}/../../benchmark"
)

find_path(GOOGLE_BENCHMARK_SRC_DIR
  NAMES benchmark.cc
  PATH_SUFFIXES src
  HINTS
    "${GOOGLEBENCHMARK_DIR}" ENV GOOGLEBENCHMARK_DIR
    "${CMAKE_SOURCE_DIR}/benchmark"
    "${CMAKE_SOURCE_DIR}/../benchmark"
    "${CMAKE_SOURCE_DIR}/../../benchmark"
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GoogleBenchmark
  DEFAULT_MSG
    GOOGLE_BENCHMARK_INCLUDE_DIR GOOGLE_BENCHMARK_SRC_DIR
)

if(GoogleBenchmark_FOUND AND NOT TARGET GoogleBenchmark)
  add_library(GoogleBenchmark STATIC
    "${GOOGLE_BENCHMARK_SRC_DIR}/benchmark.cc"
    "${GOOGLE_BENCHMARK_SRC_DIR}/benchmark_api_internal.cc"
    "${GOOGLE_BENCHMARK_SRC_DIR}/benchmark_name.cc"
    "${GOOGLE_BENCHMARK_SRC_DIR}/benchmark_register.cc"
    "${GOOGLE_BENCHMARK_SRC_DIR}/benchmark_runner.cc"
    "${GOOGLE_BENCHMARK_SRC_DIR}/colorprint.cc"
    "${GOOGLE_BENCHMARK_SRC_DIR}/commandlineflags.cc"
    "${GOOGLE_BENCHMARK_SRC_DIR}/complexity.cc"
    "${GOOGLE_BENCHMARK_SRC_DIR}/console_reporter.cc"
    "${GOOGLE_BENCHMARK_SRC_DIR}/counter.cc"
    "${GOOGLE_BENCHMARK_SRC_DIR}/csv_reporter.cc"
    "${GOOGLE_BENCHMARK_SRC_DIR}/json_reporter.cc"
    "${GOOGLE_BENCHMARK_SRC_DIR}/reporter.cc"
    "${GOOGLE_BENCHMARK_SRC_DIR}/sleep.cc"
    "${GOOGLE_BENCHMARK_SRC_DIR}/statistics.cc"
    "${GOOGLE_BENCHMARK_SRC_DIR}/string_util.cc"
    "${GOOGLE_BENCHMARK_SRC_DIR}/sysinfo.cc"
    "${GOOGLE_BENCHMARK_SRC_DIR}/timers.cc"
  )
  target_include_directories(GoogleBenchmark
    PUBLIC
      "${GOOGLE_BENCHMARK_INCLUDE_DIR}"
    PRIVATE
      "${GOOGLE_BENCHMARK_SRC_DIR}"
  )
  target_compile_definitions(GoogleBenchmark PRIVATE HAVE_STD_REGEX WITH_BENCHMARKS)
  if (WIN32)
    target_link_libraries(GoogleBenchmark PRIVATE shlwapi)
  endif()
endif()

mark_as_advanced(GOOGLE_BENCHMARK_INCLUDE_DIR GOOGLE_BENCHMARK_SRC_DIR)

include(FeatureSummary)
set_package_properties(GoogleBenchmark PROPERTIES
  URL "https://github.com/google/benchmark"
  DESCRIPTION "A microbenchmark support library")
