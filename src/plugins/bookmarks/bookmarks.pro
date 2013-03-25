TEMPLATE = lib
TARGET   = Bookmarks

include(../../qtcreatorplugin.pri)
include(bookmarks_dependencies.pri)

HEADERS += bookmarksplugin.h \
           bookmark.h \
           bookmarkmanager.h \
           bookmarks_global.h

SOURCES += bookmarksplugin.cpp \
           bookmark.cpp \
           bookmarkmanager.cpp

RESOURCES += bookmarks.qrc
