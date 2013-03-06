SOURCES += $$PWD/backtrace.cpp \
           $$PWD/cdbsymbolpathlisteditor.cpp \
           $$PWD/hostutils.cpp

HEADERS += $$PWD/backtrace.h \
           $$PWD/cdbsymbolpathlisteditor.h \
           $$PWD/hostutils.h

INCLUDEPATH += $$PWD

SOURCES += $$PWD/peutils.cpp
HEADERS += $$PWD/peutils.h

win32-msvc* {
#   For the Privilege manipulation functions in sharedlibraryinjector.cpp.
    LIBS += -ladvapi32
}
