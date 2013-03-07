SOURCES += $$PWD/backtrace.cpp \
           $$PWD/cdbsymbolpathlisteditor.cpp \
           $$PWD/hostutils.cpp \
           $$PWD/peutils.cpp

HEADERS += $$PWD/backtrace.h \
           $$PWD/cdbsymbolpathlisteditor.h \
           $$PWD/hostutils.h \
           $$PWD/peutils.h

INCLUDEPATH += $$PWD

win32-msvc* {
#   For the Privilege manipulation functions in sharedlibraryinjector.cpp.
    LIBS += -ladvapi32
}
