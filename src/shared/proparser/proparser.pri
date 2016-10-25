VPATH += $$PWD
QT += xml

INCLUDEPATH *= $$PWD $$PWD/..
DEPENDPATH *= $$PWD $$PWD/..

# Input
HEADERS += \
        qmake_global.h \
        qmakeglobals.h \
        qmakeparser.h \
        qmakeevaluator.h \
        qmakeevaluator_p.h \
        profileevaluator.h \
        proitems.h \
        prowriter.h \
        qmakevfs.h \
        ioutils.h

SOURCES += \
        qmakeglobals.cpp \
        qmakeparser.cpp \
        qmakeevaluator.cpp \
        profileevaluator.cpp \
        qmakebuiltins.cpp \
        proitems.cpp \
        prowriter.cpp \
        qmakevfs.cpp \
        ioutils.cpp

RESOURCES += proparser.qrc
DEFINES += QMAKE_BUILTIN_PRFS QMAKE_OVERRIDE_PRFS
