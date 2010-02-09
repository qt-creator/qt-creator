TEMPLATE = lib
TARGET = QmlDesigner

include(../../qtcreatorplugin.pri)
include(qmldesigner_dependencies.pri)

include(core/core.pri)
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
    designmode.h \
    designmodewidget.h \
    application.h \
    designersettings.h \
    settingspage.h
SOURCES += qmldesignerplugin.cpp \
    designmode.cpp \
    designmodewidget.cpp \
    application.cpp \
    designersettings.cpp \
    settingspage.cpp
FORMS += settingspage.ui

OTHER_FILES += QmlDesigner.pluginspec
RESOURCES += qmldesignerplugin.qrc
