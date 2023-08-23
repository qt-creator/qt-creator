set(GOOGLETEST_DIR ${CMAKE_CURRENT_LIST_DIR}/../3rdparty/googletest)

if(EXISTS "${GOOGLETEST_DIR}/googletest")
  set(GOOGLETEST_SUBMODULE_IS_CHECKED_OUT YES)
else()
  set(GOOGLETEST_SUBMODULE_IS_CHECKED_OUT NO)
endif()

add_qtc_library(Googletest STATIC
  CONDITION GOOGLETEST_SUBMODULE_IS_CHECKED_OUT
  DEPENDS
    Threads::Threads
  PUBLIC_SYSTEM_INCLUDES
    "${GOOGLETEST_DIR}/googletest/include"
    "${GOOGLETEST_DIR}/googlemock/include"
  INCLUDES
    "${GOOGLETEST_DIR}/googletest"
    "${GOOGLETEST_DIR}/googlemock"
  PROPERTIES
    AUTOMOC OFF AUTOUIC OFF QT_COMPILE_OPTIONS_DISABLE_WARNINGS ON
    POSITION_INDEPENDENT_CODE ON
  URL "https://github.com/google/googletest"
  DESCRIPTION "Google Testing and Mocking Framework"
  SOURCES
    "${GOOGLETEST_DIR}/googletest/src/gtest-all.cc"
    "${GOOGLETEST_DIR}/googlemock/src/gmock-all.cc"
)
