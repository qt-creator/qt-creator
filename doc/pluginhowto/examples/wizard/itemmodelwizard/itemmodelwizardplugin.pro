#QTC_SOURCE = C:/Work/QtCreator
#QTC_BUILD = C:/Work/QtCreator/build
QTC_SOURCE = ../../../../..
QTC_BUILD = ../../../../..
TEMPLATE = lib
TARGET = ItemModelWizard
IDE_SOURCE_TREE = $$QTC_SOURCE
IDE_BUILD_TREE = $$QTC_BUILD
PROVIDER = FooCompanyInc

include($$QTC_SOURCE/src/qtcreatorplugin.pri)
include($$QTC_SOURCE/src/plugins/coreplugin/coreplugin.pri)
include($$QTC_SOURCE/src/plugins/texteditor/texteditor.pri)
include($$QTC_SOURCE/src/plugins/cppeditor/cppeditor.pri)

LIBS += -L$$IDE_PLUGIN_PATH/Nokia

HEADERS = itemmodelwizardplugin.h \
    modelnamepage.h \
    modelclasswizard.h
SOURCES = itemmodelwizardplugin.cpp \
    modelnamepage.cpp \
    modelclasswizard.cpp
OTHER_FILES = ItemModelWizard.pluginspec
FORMS += modelnamepage.ui

RESOURCES += template.qrc
