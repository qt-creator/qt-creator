extend_qtc_test(unittest
  CONDITION
    TARGET QmlProjectManagerLib
  DEPENDS
    QmlProjectManagerLib
  SOURCES
    converters-test.cpp
    projectitem-test.cpp
)

unittest_copy_data_folder()
