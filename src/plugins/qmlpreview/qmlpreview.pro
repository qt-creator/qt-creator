DEFINES += QMLPREVIEW_LIBRARY

include(../../qtcreatorplugin.pri)
include(qmlpreview_dependencies.pri)

equals(TEST, 1) {
include(tests/tests.pri)
}

HEADERS += \
    qmlpreview_global.h \
    qmldebugtranslationclient.h \
    qmldebugtranslationwidget.h \
    qmlpreviewclient.h \
    qmlpreviewplugin.h \
    qmlpreviewruncontrol.h \
    qmlpreviewconnectionmanager.h \
    qmlpreviewfileontargetfinder.h \
    projectfileselectionswidget.h

SOURCES += \
    qmlpreviewplugin.cpp \
    qmldebugtranslationclient.cpp \
    qmldebugtranslationwidget.cpp \
    qmlpreviewclient.cpp \
    qmlpreviewruncontrol.cpp \
    qmlpreviewconnectionmanager.cpp \
    qmlpreviewfileontargetfinder.cpp \
    projectfileselectionswidget.cpp

OTHER_FILES += \
    QmlPreview.json.in
