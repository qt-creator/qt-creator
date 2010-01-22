VPATH += $$PWD
QT += xml

INCLUDEPATH *= $$PWD $$PWD/..
DEPENDPATH *= $$PWD $$PWD/..

# Input
HEADERS += \
        abstractproitemvisitor.h \
        profileevaluator.h \
        proitems.h \
        prowriter.h \
        ioutils.h \
        $$PWD/../namespace_global.h

SOURCES += \
        profileevaluator.cpp \
        proitems.cpp \
        prowriter.cpp \
        ioutils.cpp

RESOURCES += proparser.qrc
