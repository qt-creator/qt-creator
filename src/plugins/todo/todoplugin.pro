CONFIG += release
TEMPLATE = lib
TARGET = todo
PROVIDER = vsorokin

QTC_SOURCE_DIR = /home/vass/qt-creator
IDE_SOURCE_TREE = $$QTC_SOURCE_DIR
QTC_BUILD_DIR = /opt/qtcreator-2.1.81

DESTDIR = $$QTC_BUILD_DIR/lib/qtcreator/plugins/$$(PROVIDER)
IDE_BUILD_TREE = $$QTC_BUILD_DIR

LIBS += -L$$IDE_BUILD_TREE/lib/qtcreator/ \
        -L$$IDE_BUILD_TREE/lib/qtcreator/plugins/Nokia

include( $$IDE_SOURCE_TREE/src/qtcreatorplugin.pri )
include( $$IDE_SOURCE_TREE/src/plugins/coreplugin/coreplugin.pri )
include( $$IDE_SOURCE_TREE/src/plugins/projectexplorer/projectexplorer.pri )
include( $$IDE_SOURCE_TREE/src/plugins/texteditor/texteditor.pri )

INCLUDEPATH += $$IDE_SOURCE_TREE/src \
               $$IDE_SOURCE_TREE/src/plugins \
               $$IDE_SOURCE_TREE/src/libs \
               $$IDE_SOURCE_TREE/src/libs/extensionsystem


HEADERS += todoplugin.h \
    todooutputpane.h \
    settingsdialog.h \
    settingspage.h \
    addkeyworddialog.h \
    keyword.h
SOURCES += todoplugin.cpp \
    todooutputpane.cpp \
    settingsdialog.cpp \
    settingspage.cpp \
    addkeyworddialog.cpp \
    keyword.cpp
OTHER_FILES += todo.pluginspec

RESOURCES += \
    icons.qrc

FORMS += \
    settingsdialog.ui \
    addkeyworddialog.ui
