DEFINES+=CPLUSPLUS_BUILD_STATIC_LIB
include(../../../../qtcreator.pri)
include($$IDE_SOURCE_TREE/src/libs/cplusplus/cplusplus.pri)
INCLUDEPATH += $$IDE_SOURCE_TREE/src/libs/cplusplus
LIBS += -L$$OUT_PWD
