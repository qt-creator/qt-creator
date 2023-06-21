set(GOOGLETEST_DIR ${CMAKE_CURRENT_LIST_DIR}/../3rdparty/googletest)

if (EXISTS "${GOOGLETEST_DIR}/googletest")
  add_library(Googletest STATIC
    "${GOOGLETEST_DIR}/googletest/src/gtest-all.cc"
    "${GOOGLETEST_DIR}/googlemock/src/gmock-all.cc"
  )
  target_include_directories(Googletest
    PUBLIC
      "${GOOGLETEST_DIR}/googletest/include"
      "${GOOGLETEST_DIR}/googlemock/include"
    PRIVATE
      "${GOOGLETEST_DIR}/googletest"
      "${GOOGLETEST_DIR}/googlemock"
  )
  set_target_properties(Googletest PROPERTIES AUTOMOC OFF AUTOUIC OFF QT_COMPILE_OPTIONS_DISABLE_WARNINGS ON)
  set_property(TARGET Googletest PROPERTY POSITION_INDEPENDENT_CODE ON)

  target_link_libraries(Googletest Threads::Threads)
else()
  message(STATUS "Googletest was not found. Please update the submodules with `git submodule update --init --recursive`.")
  message(STATUS "Otherwise tests that depend on Googletest will be skipped.")
  return()
endif()

include(FeatureSummary)
set_package_properties(Googletest PROPERTIES
  URL "https://github.com/google/googletest"
  DESCRIPTION "Google Testing and Mocking Framework")
