TEMPLATE = lib
TARGET = TaskList

include(../../qtcreatorplugin.pri)
include(../../plugins/coreplugin/coreplugin.pri)
include(../../plugins/projectexplorer/projectexplorer.pri)

HEADERS += tasklistplugin.h \
    taskfilefactory.h \
    tasklistmanager.h

SOURCES += tasklistplugin.cpp \
    taskfilefactory.cpp \
    tasklistmanager.cpp

RESOURCES += tasklist.qrc

OTHER_FILES += TaskList.pluginspec \
    TaskList.mimetypes.xml
