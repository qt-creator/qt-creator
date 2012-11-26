TEMPLATE = lib
TARGET = TaskList

include(../../qtcreatorplugin.pri)
include(../../plugins/coreplugin/coreplugin.pri)
include(../../plugins/projectexplorer/projectexplorer.pri)

DEFINES += QT_NO_CAST_FROM_ASCII

HEADERS += tasklistplugin.h \
    tasklist_export.h \
    tasklistconstants.h \
    stopmonitoringhandler.h \
    taskfile.h \
    taskfilefactory.h \

SOURCES += tasklistplugin.cpp \
    stopmonitoringhandler.cpp \
    taskfile.cpp \
    taskfilefactory.cpp \

RESOURCES += tasklist.qrc

OTHER_FILES += TaskList.mimetypes.xml
