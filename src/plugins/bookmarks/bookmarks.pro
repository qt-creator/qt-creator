TEMPLATE = lib
TARGET   = Bookmarks

include(../../qtcreatorplugin.pri)
include(../../plugins/projectexplorer/projectexplorer.pri)
include(../../plugins/coreplugin/coreplugin.pri)
include(../../plugins/texteditor/texteditor.pri)

HEADERS += bookmarksplugin.h \
           bookmark.h \
           bookmarkmanager.h \
           bookmarks_global.h

SOURCES += bookmarksplugin.cpp \
           bookmark.cpp \
           bookmarkmanager.cpp

RESOURCES += bookmarks.qrc
