# QtCreator Utils Tests
extend_qtc_test(unittest
    SOURCES
        matchingtext-test.cpp
        sizedarray-test.cpp
        smallstring-test.cpp
)

extend_qtc_test(unittest
  CONDITION TARGET GoogleBenchmark
  DEPENDS GoogleBenchmark
  SOURCES
    smallstring-benchmark.cpp
)

unittest_copy_data_folder()
