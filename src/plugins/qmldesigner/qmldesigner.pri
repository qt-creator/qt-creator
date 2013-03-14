include(qmldesigner_dependencies.pri)
include(designercore/designercore.pri)
LIBS *= -l$$qtLibraryName(QmlDesigner)
INCLUDEPATH *= $$PWD
INCLUDEPATH *= $$PWD/components/componentcore
INCLUDEPATH *= $$PWD/components/formeditor
INCLUDEPATH *= $$PWD/components/itemlibrary
INCLUDEPATH *= $$PWD/components/navigator
INCLUDEPATH *= $$PWD/components/propertyeditor
INCLUDEPATH *= $$PWD/components/stateseditor
INCLUDEPATH *= $$PWD/components/debugview
INCLUDEPATH *= $$PWD/components/integration
INCLUDEPATH *= $$PWD/components/logger
INCLUDEPATH *= $$QTCREATOR_SOURCES/share/qtcreator/qml/qmlpuppet/interfaces
