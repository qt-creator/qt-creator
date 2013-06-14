include(../../qtcreatorplugin.pri)

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
