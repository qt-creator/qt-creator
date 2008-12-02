TEMPLATE = lib
TARGET = QHelpProject

include(../../qworkbenchplugin.pri)
include(../../../../helpsystem/qhelpsystem.pri)
include(../../plugins/coreplugin/coreplugin.pri)

SOURCES += qhelpprojectmanager.cpp \
           qhelpproject.cpp \
           qhelpprojectitems.cpp


HEADERS += qhelpprojectmanager.h \
           qhelpproject.h \
           qhelpprojectitems.h

