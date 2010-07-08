DEFINES+=CPLUSPLUS_BUILD_STATIC_LIB
include(../../../../qtcreator.pri)
INCLUDEPATH += $$IDE_SOURCE_TREE/src/libs/cplusplus
INCLUDEPATH += $$IDE_SOURCE_TREE/src/shared/cplusplus
LIBS += -L$$OUT_PWD/../shared -lCPlusPlusTestSupport
