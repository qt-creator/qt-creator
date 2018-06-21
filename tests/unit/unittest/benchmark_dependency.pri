GOOGLEBENCHMARK_DIR = $$(GOOGLEBENCHMARK_DIR)

exists($$GOOGLEBENCHMARK_DIR) {
  INCLUDEPATH += $$GOOGLEBENCHMARK_DIR/include

  DEFINES += HAVE_STD_REGEX WITH_BENCHMARKS
  SOURCES += \
      $$GOOGLEBENCHMARK_DIR/src/benchmark.cc \
      $$GOOGLEBENCHMARK_DIR/src/benchmark_register.cc \
      $$GOOGLEBENCHMARK_DIR/src/colorprint.cc \
      $$GOOGLEBENCHMARK_DIR/src/commandlineflags.cc \
      $$GOOGLEBENCHMARK_DIR/src/complexity.cc \
      $$GOOGLEBENCHMARK_DIR/src/console_reporter.cc \
      $$GOOGLEBENCHMARK_DIR/src/counter.cc \
      $$GOOGLEBENCHMARK_DIR/src/csv_reporter.cc \
      $$GOOGLEBENCHMARK_DIR/src/json_reporter.cc \
      $$GOOGLEBENCHMARK_DIR/src/reporter.cc \
      $$GOOGLEBENCHMARK_DIR/src/sleep.cc \
      $$GOOGLEBENCHMARK_DIR/src/statistics.cc \
      $$GOOGLEBENCHMARK_DIR/src/string_util.cc \
      $$GOOGLEBENCHMARK_DIR/src/sysinfo.cc \
      $$GOOGLEBENCHMARK_DIR/src/timers.cc

   win32:LIBS += -lShlwapi
}
