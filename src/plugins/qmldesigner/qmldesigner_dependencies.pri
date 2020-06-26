QTC_PLUGIN_NAME = QmlDesigner
QTC_LIB_DEPENDS += \
    utils \
    qmljs \
    qmleditorwidgets \
    advanceddockingsystem
QTC_PLUGIN_DEPENDS += \
    coreplugin \
    texteditor \
    qmljseditor \
    qtsupport \
    projectexplorer \
    qmakeprojectmanager \
    qmlprojectmanager \
    resourceeditor

INCLUDEPATH *= \
    $$PWD \
    $$PWD/../../../share/qtcreator/qml/qmlpuppet/interfaces \
    $$PWD/../../../share/qtcreator/qml/qmlpuppet/types \
    $$PWD/designercore \
    $$PWD/designercore/include \
    $$PWD/components/integration \
    $$PWD/components/componentcore \
    $$PWD/components/importmanager \
    $$PWD/components/itemlibrary \
    $$PWD/components/edit3d \
    $$PWD/components/formeditor \
    $$PWD/components/navigator \
    $$PWD/components/stateseditor \
    $$PWD/components/texteditor \
    $$PWD/components/propertyeditor \
    $$PWD/components/debugview
