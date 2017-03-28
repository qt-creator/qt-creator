QT += quickwidgets core-private
CONFIG += exceptions

INCLUDEPATH += $$PWD

unix:!openbsd:!osx: LIBS += -lrt # posix shared memory

include(../../qtcreatorplugin.pri)

include(qmldesignerplugin.pri)
include(designercore/designercore-lib.pri)
include(components/componentcore/componentcore.pri)
include(components/integration/integration.pri)
include(components/propertyeditor/propertyeditor.pri)
include(components/formeditor/formeditor.pri)
include(components/texteditor/texteditor.pri)
include(components/itemlibrary/itemlibrary.pri)
include(components/navigator/navigator.pri)
include(components/stateseditor/stateseditor.pri)
include(components/resources/resources.pri)
include(components/debugview/debugview.pri)
include(components/importmanager/importmanager.pri)
include(qmldesignerextension/qmldesignerextension.pri)


BUILD_PUPPET_IN_CREATOR_BINPATH = $$(BUILD_PUPPET_IN_CREATOR_BINPATH)
!isEmpty(BUILD_PUPPET_IN_CREATOR_BINPATH) {
    DEFINES += SEARCH_PUPPET_IN_CREATOR_BINPATH
    message("Search puppet in qtcreator bin path!")
}
