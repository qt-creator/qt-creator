SOURCES += $$PWD/backtrace.cpp \
           $$PWD/cdbsymbolpathlisteditor.cpp \
           $$PWD/hostutils.cpp \
           $$PWD/peutils.cpp \
    shared/symbolpathsdialog.cpp

HEADERS += $$PWD/backtrace.h \
           $$PWD/cdbsymbolpathlisteditor.h \
           $$PWD/hostutils.h \
           $$PWD/peutils.h \
    shared/symbolpathsdialog.h

# For the Privilege manipulation functions in sharedlibraryinjector.cpp.
win32: LIBS += -ladvapi32

FORMS += \
    shared/symbolpathsdialog.ui
