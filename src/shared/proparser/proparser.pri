VPATH += $$PWD
QT += xml

INCLUDEPATH *= $$PWD $$PWD/..
DEPENDPATH *= $$PWD $$PWD/..

# Input
HEADERS += \
        qmake_global.h \
        profileparser.h \
        profileevaluator.h \
        proitems.h \
        prowriter.h \
        ioutils.h

SOURCES += \
        profileparser.cpp \
        profileevaluator.cpp \
        proitems.cpp \
        prowriter.cpp \
        ioutils.cpp

RESOURCES += proparser.qrc
