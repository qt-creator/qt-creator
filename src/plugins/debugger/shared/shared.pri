

SOURCES += $$PWD/backtrace.cpp
HEADERS += $$PWD/backtrace.h

win32 {

INCLUDEPATH+=$$PWD

SOURCES += $$PWD/peutils.cpp \
           $$PWD/dbgwinutils.cpp \
	   $$PWD/sharedlibraryinjector.cpp

HEADERS += $$PWD/peutils.h \
           $$PWD/dbgwinutils.h \
           $$PWD/sharedlibraryinjector.h

contains(QMAKE_CXX, cl) {
#   For the Privilege manipulation functions in sharedlibraryinjector.cpp.
#   Not required for MinGW.
    LIBS += advapi32.lib
}

}
