extend_qtc_test(unittest
  SOURCES
    propertycomponentgenerator-test.cpp
    propertyeditorcomponentgenerator-test.cpp
)

extend_qtc_test(unittest
  SOURCES_PREFIX ../../../../../src/plugins/qmldesigner/components/componentcore
  SOURCES
    propertyeditorcomponentgenerator.cpp propertyeditorcomponentgenerator.h
    propertycomponentgenerator.cpp propertycomponentgenerator.h
    propertycomponentgeneratorinterface.h
)


