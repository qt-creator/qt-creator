VPATH += $$PWD
QT += xml

INCLUDEPATH *= $$PWD $$PWD/..
DEPENDPATH *= $$PWD $$PWD/..

# Input
HEADERS += \
        profileparser.h \
        profileevaluator.h \
        proitems.h \
        prowriter.h \
        ioutils.h \
        $$PWD/../namespace_global.h

SOURCES += \
        profileparser.cpp \
        profileevaluator.cpp \
        proitems.cpp \
        prowriter.cpp \
        ioutils.cpp

RESOURCES += proparser.qrc
