set(GTEST_MAIN_SRC "../../../../src/libs/3rdparty/googletest/googletest/src/gtest_main.cc")

add_qtc_library(
  integrationtestscommon
  STATIC
  EXCLUDE_FROM_INSTALL
  SKIP_AUTOMOC
  CONDITION
    WITH_TESTS
  PUBLIC_DEPENDS
    Googletest TestDesignerCore TestUtils TestMatchers TestPrinters TestMocks
  SOURCES
    ${GTEST_MAIN_SRC}
)

include("integrationtestsapi.cmake")

add_subdirectory(qmlprojectmanager)
