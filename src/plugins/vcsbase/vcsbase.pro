TEMPLATE = lib
TARGET = VCSBase
DEFINES += VCSBASE_LIBRARY
include(../../qworkbenchplugin.pri)
include(vcsbase_dependencies.pri)
HEADERS += vcsbase_global.h \
    vcsbaseconstants.h \
    vcsbaseplugin.h \
    baseannotationhighlighter.h \
    diffhighlighter.h \
    vcsbasetextdocument.h \
    vcsbaseeditor.h \
    vcsbasesubmiteditor.h \
    basevcseditorfactory.h \
    submiteditorfile.h \
    basevcssubmiteditorfactory.h \
    submitfilemodel.h
SOURCES += vcsbaseplugin.cpp \
    baseannotationhighlighter.cpp \
    diffhighlighter.cpp \
    vcsbasetextdocument.cpp \
    vcsbaseeditor.cpp \
    vcsbasesubmiteditor.cpp \
    basevcseditorfactory.cpp \
    submiteditorfile.cpp \
    basevcssubmiteditorfactory.cpp \
    submitfilemodel.cpp
RESOURCES = vcsbase.qrc
