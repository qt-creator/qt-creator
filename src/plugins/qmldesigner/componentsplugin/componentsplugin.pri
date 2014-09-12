TARGET = componentsplugin
TEMPLATE = lib
CONFIG += plugin

QT += qml

include (../designercore/iwidgetplugin.pri)
include (../qmldesigner_dependencies.pri)

LIBS += -L$$IDE_PLUGIN_PATH
LIBS += -l$$qtLibraryName(QmlDesigner)
LIBS += -l$$qtLibraryName(Core)
LIBS += -l$$qtLibraryName(Utils)

DEFINES += COMPONENTS_LIBRARY

RESOURCES += $$PWD/componentsplugin.qrc

HEADERS += \
    $$PWD/tabviewindexmodel.h \
    $$PWD/componentsplugin.h \
    $$PWD/../designercore/include/iwidgetplugin.h \
    $$PWD/addtabdesigneraction.h \
    $$PWD/addtabtotabviewdialog.h \
    $$PWD/entertabdesigneraction.h

SOURCES += \
    $$PWD/componentsplugin.cpp \
    $$PWD/tabviewindexmodel.cpp \
    $$PWD/addtabdesigneraction.cpp \
    $$PWD/addtabtotabviewdialog.cpp \
    $$PWD/entertabdesigneraction.cpp

FORMS += \
    $$PWD/addtabtotabviewdialog.ui


DISTFILES += $$PWD/components.metainfo
