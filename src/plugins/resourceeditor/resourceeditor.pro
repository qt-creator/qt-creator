include(../../qtcreatorplugin.pri)
include(qrceditor/qrceditor.pri)

HEADERS += resourceeditorfactory.h \
resourceeditorplugin.h \
resourceeditorw.h \
resourceeditorconstants.h \
resource_global.h \
resourcenode.h

SOURCES +=resourceeditorfactory.cpp \
resourceeditorplugin.cpp \
resourceeditorw.cpp \
resourcenode.cpp

DEFINES += RESOURCE_LIBRARY
