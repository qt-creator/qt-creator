DEFINES+=CPLUSPLUS_BUILD_STATIC_LIB
INCLUDEPATH += $$PWD/../../../../src/shared/cplusplus
INCLUDEPATH += $$PWD/../../../../src/libs/cplusplus
DEPENDPATH  += $$INCLUDEPATH .
LIBS += -L$$PWD -lCPlusPlusTestSupport
