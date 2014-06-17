TARGET = componentsplugin
TEMPLATE = lib
CONFIG += plugin

QT += qml

include (../designercore/iwidgetplugin.pri)
include (../qmldesigner_dependencies.pri)

LIBS += -L$$IDE_PLUGIN_PATH
LIBS += -l$$qtLibraryName(QmlDesigner)

DEFINES += COMPONENTS_LIBRARY

RESOURCES += $$PWD/componentsplugin.qrc

HEADERS += \
    $$PWD/tabviewindexmodel.h \
    $$PWD/componentsplugin.h \
    $$PWD/../designercore/include/iwidgetplugin.h

SOURCES += \
    $$PWD/componentsplugin.cpp \
    $$PWD/tabviewindexmodel.cpp


OTHER_FILES += $$PWD/components.metainfo
