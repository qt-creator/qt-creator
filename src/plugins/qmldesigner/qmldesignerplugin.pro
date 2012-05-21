TEMPLATE = lib
TARGET = QmlDesigner

CONFIG += exceptions

INCLUDEPATH += $$PWD

include(../../qtcreatorplugin.pri)
include(../../private_headers.pri)
include(qmldesigner_dependencies.pri)

include(designercore/designercore.pri)
include(components/integration/integration.pri)
include(components/propertyeditor/propertyeditor.pri)
include(components/formeditor/formeditor.pri)
include(components/itemlibrary/itemlibrary.pri)
include(components/navigator/navigator.pri)
include(components/pluginmanager/pluginmanager.pri)
include(components/themeloader/qts60stylethemeio.pri)
include(components/stateseditor/stateseditor.pri)
include(components/resources/resources.pri)

HEADERS += qmldesignerconstants.h \
    qmldesignerplugin.h \
    designmodewidget.h \
    designersettings.h \
    settingspage.h \
    designmodecontext.h \
    styledoutputpaneplaceholder.h

SOURCES += qmldesignerplugin.cpp \
    designmodewidget.cpp \
    designersettings.cpp \
    settingspage.cpp \
    designmodecontext.cpp \
    styledoutputpaneplaceholder.cpp

FORMS += settingspage.ui
