add_qtc_library(TestMatchers OBJECT
  EXCLUDE_FROM_INSTALL
  SKIP_AUTOMOC ON
  PUBLIC_INCLUDES ${CMAKE_CURRENT_LIST_DIR}
  DEPENDS
    Googletest Utils
  SOURCES
    dynamicastmatcherdiagnosticcontainer-matcher.h
    unittest-matchers.h
)
