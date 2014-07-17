DEPENDPATH += $$PWD
INCLUDEPATH += $$PWD

DEFINES *= QT_NO_CAST_FROM_ASCII
DEFINES += PATH_PREPROCESSOR_CONFIG=\\\"$$PWD/pp-configuration.inc\\\"
DEFINES += QTCREATOR_UTILS_STATIC_LIB

HEADERS += \
    $$PWD/cplusplus-tools-utils.h \
    $$PWD/../../libs/utils/environment.h \
    $$PWD/../../libs/utils/fileutils.h \
    $$PWD/../../libs/utils/qtcassert.h \
    $$PWD/../../libs/utils/savefile.h

SOURCES += \
    $$PWD/cplusplus-tools-utils.cpp \
    $$PWD/../../libs/utils/environment.cpp \
    $$PWD/../../libs/utils/fileutils.cpp \
    $$PWD/../../libs/utils/qtcassert.cpp \
    $$PWD/../../libs/utils/savefile.cpp

win32:LIBS += -luser32 -lshell32
