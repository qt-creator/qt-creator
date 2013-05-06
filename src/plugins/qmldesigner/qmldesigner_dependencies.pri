QTC_PLUGIN_NAME = QmlDesigner
QTC_LIB_DEPENDS += \
    utils \
    qmljs \
    qmleditorwidgets
QTC_PLUGIN_DEPENDS += \
    coreplugin \
    texteditor \
    qmljseditor \
    qt4projectmanager \
    qmlprojectmanager \
    projectexplorer
INCLUDEPATH *= \
    $$PWD/components/componentcore \
    $$PWD/components/formeditor \
    $$PWD/components/itemlibrary \
    $$PWD/components/navigator \
    $$PWD/components/propertyeditor \
    $$PWD/components/stateseditor \
    $$PWD/components/debugview \
    $$PWD/components/integration \
    $$PWD/components/logger
