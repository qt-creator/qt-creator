#.rst:
# FindGoogletest
# -----------------
#
# Try to locate the Googletest source files, and then build them as a
# static library.
#
# The ``GOOGLETEST_DIR`` (CMake or Environment) variable should be used
# to pinpoint the Googletest source files.
#
# If found, this will define the following variables:
#
# ``Googletest_FOUND``
#     True if the Googletest source package has been found.
#
# ``Googletest``
#     Target compiled as static library.
#

find_path(GOOGLE_TEST_INCLUDE_DIR
  NAMES gtest/gtest.h
  PATH_SUFFIXES googletest/include
  HINTS
    "${GOOGLETEST_DIR}" ENV GOOGLETEST_DIR
    "${CMAKE_SOURCE_DIR}/googletest"
    "${CMAKE_SOURCE_DIR}/../googletest"
    "${CMAKE_SOURCE_DIR}/../../googletest"
)

find_path(GOOGLE_TEST_SRC_ALL
  NAMES gtest-all.cc
  PATH_SUFFIXES googletest/src
  HINTS
    "${GOOGLETEST_DIR}" ENV GOOGLETEST_DIR
    "${CMAKE_SOURCE_DIR}/googletest"
    "${CMAKE_SOURCE_DIR}/../googletest"
    "${CMAKE_SOURCE_DIR}/../../googletest"
)


find_path(GOOGLE_MOCK_INCLUDE_DIR
  NAMES gmock/gmock.h
  PATH_SUFFIXES googlemock/include
  HINTS
    "${GOOGLETEST_DIR}" ENV GOOGLETEST_DIR
    "${CMAKE_SOURCE_DIR}/googletest"
    "${CMAKE_SOURCE_DIR}/../googletest"
    "${CMAKE_SOURCE_DIR}/../../googletest"
)

find_path(GOOGLE_MOCK_SRC_ALL
  NAMES gmock-all.cc
  PATH_SUFFIXES googlemock/src
  HINTS
    "${GOOGLETEST_DIR}" ENV GOOGLETEST_DIR
    "${CMAKE_SOURCE_DIR}/googletest"
    "${CMAKE_SOURCE_DIR}/../googletest"
    "${CMAKE_SOURCE_DIR}/../../googletest"
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Googletest
  DEFAULT_MSG
    GOOGLE_TEST_INCLUDE_DIR GOOGLE_MOCK_INCLUDE_DIR
    GOOGLE_TEST_SRC_ALL GOOGLE_MOCK_SRC_ALL
)

if(Googletest_FOUND AND NOT TARGET Googletest)
  add_library(Googletest STATIC
    "${GOOGLE_TEST_SRC_ALL}/gtest-all.cc"
    "${GOOGLE_MOCK_SRC_ALL}/gmock-all.cc"
  )
  target_include_directories(Googletest
    PUBLIC
      "${GOOGLE_TEST_INCLUDE_DIR}"
      "${GOOGLE_MOCK_INCLUDE_DIR}"
    PRIVATE
      "${GOOGLE_TEST_SRC_ALL}/.."
      "${GOOGLE_MOCK_SRC_ALL}/.."
  )
  target_compile_definitions(Googletest
    PRIVATE
      GTEST_HAS_STD_INITIALIZER_LIST_
      GTEST_LANG_CXX11
      GTEST_HAS_STD_TUPLE_
      GTEST_HAS_STD_TYPE_TRAITS_
      GTEST_HAS_STD_FUNCTION_
      GTEST_HAS_RTTI
      GTEST_HAS_STD_BEGIN_AND_END_
      GTEST_HAS_STD_UNIQUE_PTR_
      GTEST_HAS_EXCEPTIONS
      GTEST_HAS_STREAM_REDIRECTION
      GTEST_HAS_TYPED_TEST
      GTEST_HAS_TYPED_TEST_P
      GTEST_HAS_PARAM_TEST
      GTEST_HAS_DEATH_TEST
   )
endif()

mark_as_advanced(GOOGLE_TEST_INCLUDE_DIR GOOGLE_MOCK_INCLUDE_DIR
  GOOGLE_TEST_SRC_ALL GOOGLE_MOCK_SRC_ALL)

include(FeatureSummary)
set_package_properties(Googletest PROPERTIES
  URL "https://github.com/google/googletest"
  DESCRIPTION "Google Testing and Mocking Framework")
