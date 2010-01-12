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
        proxml.h \
        $$PWD/../namespace_global.h

SOURCES += \
        profileevaluator.cpp \
        proitems.cpp \
        prowriter.cpp \
        proxml.cpp

RESOURCES += proparser.qrc
