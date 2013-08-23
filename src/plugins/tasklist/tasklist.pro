include(../../qtcreatorplugin.pri)

HEADERS += tasklistplugin.h \
    tasklistconstants.h \
    stopmonitoringhandler.h \
    taskfile.h \
    taskfilefactory.h \

SOURCES += tasklistplugin.cpp \
    stopmonitoringhandler.cpp \
    taskfile.cpp \
    taskfilefactory.cpp \

RESOURCES += tasklist.qrc
