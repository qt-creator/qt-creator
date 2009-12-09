VPATH += $$PWD
QT += xml

INCLUDEPATH *= $$PWD $$PWD/..
DEPENDPATH *= $$PWD $$PWD/..

# Input
HEADERS += \
        abstractproitemvisitor.h \
        procommandmanager.h \
        proeditormodel.h \
        profileevaluator.h \
        proitems.h \
        prowriter.h \
        proxml.h \
        $$PWD/../namespace_global.h

SOURCES += \
        procommandmanager.cpp \
        proeditormodel.cpp \
        profileevaluator.cpp \
        proitems.cpp \
        prowriter.cpp \
        proxml.cpp

RESOURCES += proparser.qrc
