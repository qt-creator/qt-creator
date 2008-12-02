TEMPLATE = lib
TARGET   = Snippets
QT      += xml

include(../../qworkbenchplugin.pri)
include(../../plugins/projectexplorer/projectexplorer.pri)
include(../../plugins/coreplugin/coreplugin.pri)
include(../../plugins/texteditor/texteditor.pri)

INCLUDEPATH += ../projectexplorer

HEADERS += snippetsplugin.h \
           snippetswindow.h \
           snippetspec.h \
           snippetscompletion.h \
           inputwidget.h 

SOURCES += snippetsplugin.cpp \
           snippetswindow.cpp \
           snippetspec.cpp \
           snippetscompletion.cpp \
           inputwidget.cpp 

RESOURCES += snippets.qrc
