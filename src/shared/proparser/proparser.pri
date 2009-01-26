VPATH += $$PWD
QT += xml

INCLUDEPATH *= $$PWD $$PWD/..

# Input
HEADERS += \
        abstractproitemvisitor.h \
        procommandmanager.h \
        proeditor.h \
        proeditormodel.h \
        profileevaluator.h \
        proiteminfo.h \
        proitems.h \
        proparserutils.h \
        prowriter.h \
        proxml.h \
        valueeditor.h \
        $$PWD/../namespace_global.h

SOURCES += \
        procommandmanager.cpp \
        proeditor.cpp \
        proeditormodel.cpp \
        profileevaluator.cpp \
        proiteminfo.cpp \
        proitems.cpp \
        prowriter.cpp \
        proxml.cpp \
        valueeditor.cpp

FORMS 	+= proeditor.ui \
        valueeditor.ui

RESOURCES += proparser.qrc
