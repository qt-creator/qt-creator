INCLUDEPATH+=$$PWD
SOURCES += $$PWD/peutils.cpp \
           $$PWD/dbgwinutils.cpp \
	   $$PWD/sharedlibraryinjector.cpp

HEADERS += $$PWD/peutils.h \
           $$PWD/dbgwinutils.h \
           $$PWD/sharedlibraryinjector.h

# For the Privilege manipulation functions in sharedlibraryinjector.cpp
LIBS += advapi32.lib
