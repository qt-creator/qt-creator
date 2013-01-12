TEMPLATE = lib
TARGET = ResourceEditor

include(../../qtcreatorplugin.pri)
include(../../libs/utils/utils.pri)
include(../../plugins/coreplugin/coreplugin.pri)
include(../../plugins/find/find.pri)
include(qrceditor/qrceditor.pri)

INCLUDEPATH += $$PWD/../../tools/utils

HEADERS += resourceeditorfactory.h \
resourceeditorplugin.h \
resourcewizard.h \
resourceeditorw.h \
resourceeditorconstants.h

SOURCES +=resourceeditorfactory.cpp \
resourceeditorplugin.cpp \
resourcewizard.cpp \
resourceeditorw.cpp

RESOURCES += resourceeditor.qrc
