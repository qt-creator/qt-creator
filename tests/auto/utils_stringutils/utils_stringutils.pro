include(../qttest.pri)
include($$IDE_SOURCE_TREE/src/libs/utils/utils.pri)

DEFINES -= QT_USE_FAST_OPERATOR_PLUS QT_USE_FAST_CONCATENATION

SOURCES += tst_stringutils.cpp \
#    $$UTILS_PATH/stringutils.cpp

#HEADERS += $$UTILS_PATH/stringutils.h \
#    $$UTILS_PATH/utils_global.h
