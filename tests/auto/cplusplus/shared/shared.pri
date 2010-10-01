DEFINES+=CPLUSPLUS_BUILD_STATIC_LIB
include(../../../../qtcreator.pri)
INCLUDEPATH += $$IDE_SOURCE_TREE/src/libs/cplusplus
INCLUDEPATH += $$IDE_SOURCE_TREE/src/shared/cplusplus
include($$IDE_SOURCE_TREE/src/libs/cplusplus/cplusplus-lib.pri)
include($$IDE_SOURCE_TREE/src/libs/utils/utils-lib.pri)
