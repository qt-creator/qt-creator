QTC_SOURCE = C:/Work/QtCreator
QTC_BUILD = C:/Work/QtCreator/build

TEMPLATE = lib
TARGET = HTMLEditor
IDE_SOURCE_TREE = $$QTC_SOURCE
IDE_BUILD_TREE = $$QTC_BUILD
PROVIDER = FooCompanyInc

QT += webkit

PROVIDER = FooCompanyInc

include($$QTC_SOURCE/src/qtcreatorplugin.pri)
include($$QTC_SOURCE/src/plugins/coreplugin/coreplugin.pri)
LIBS += -L$$IDE_PLUGIN_PATH/Nokia

HEADERS = htmleditorplugin.h \
    htmleditorwidget.h \
    htmlfile.h \
    htmleditor.h \
    htmleditorfactory.h

SOURCES = htmleditorplugin.cpp \
    htmleditorwidget.cpp \
    htmlfile.cpp \
    htmleditor.cpp \
    htmleditorfactory.cpp

OTHER_FILES = HTMLEditor.pluginspec \
              text-html-mimetype.xml
