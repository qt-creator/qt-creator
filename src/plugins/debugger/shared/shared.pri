SOURCES += $$PWD/backtrace.cpp \
           $$PWD/cdbsymbolpathlisteditor.cpp

HEADERS += $$PWD/backtrace.h \
           $$PWD/cdbsymbolpathlisteditor.h

INCLUDEPATH+=$$PWD

win32 {
SOURCES += $$PWD/peutils.cpp \
           $$PWD/dbgwinutils.cpp

HEADERS += $$PWD/peutils.h \
           $$PWD/dbgwinutils.h

contains(QMAKE_CXX, cl) {
#   For the Privilege manipulation functions in sharedlibraryinjector.cpp.
    LIBS += -ladvapi32
}

}
