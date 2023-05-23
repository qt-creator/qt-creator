extend_qtc_test(unittest
  CONDITION Qt6_VERSION VERSION_GREATER_EQUAL 6.4.3
  DEPENDS
    QmlProjectManagerLib
  SOURCES
    converters-test.cpp
    projectitem-test.cpp
)
