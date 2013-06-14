include(../../qtcreatorplugin.pri)
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
