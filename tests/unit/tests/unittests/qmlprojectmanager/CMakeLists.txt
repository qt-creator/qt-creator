extend_qtc_test(unittest
  CONDITION
    TARGET QmlProjectManagerLib
  DEPENDS
    QmlProjectManagerLib
  SOURCES
    projectitem-test.cpp
    mcumoduleprojectitem-test.cpp
)

unittest_copy_data_folder()
