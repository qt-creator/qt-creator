include(../shared/shared.pri)

# Inject the source dir for referencing test data from shadow builds.
DEFINES += SRCDIR=\\\"$$PWD\\\"

SOURCES += tst_cppselectionchangertest.cpp

DISTFILES += testCppFile.cpp \
    cppselectionchanger.qbs
