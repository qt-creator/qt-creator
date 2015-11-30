QT += quickwidgets
QT += widgets-private quick-private quickwidgets-private core-private gui-private #mouse ungrabbing workaround on quickitems
CONFIG += exceptions

INCLUDEPATH += $$PWD

unix:!osx:LIBS += -lrt # posix shared memory

include(../../qtcreatorplugin.pri)

include(designercore/designercore-lib.pri)
include(components/componentcore/componentcore.pri)
include(components/integration/integration.pri)
include(components/propertyeditor/propertyeditor.pri)
include(components/formeditor/formeditor.pri)
include(components/itemlibrary/itemlibrary.pri)
include(components/navigator/navigator.pri)
include(components/pluginmanager/pluginmanager.pri)
include(components/stateseditor/stateseditor.pri)
include(components/resources/resources.pri)
include(components/debugview/debugview.pri)
include(components/importmanager/importmanager.pri)
include(qmldesignerextension/qmldesignerextension.pri)
include(qmldesignerplugin.pri)

DEFINES -= QT_NO_CAST_FROM_ASCII

BUILD_PUPPET_IN_CREATOR_BINPATH = $$(BUILD_PUPPET_IN_CREATOR_BINPATH)
!isEmpty(BUILD_PUPPET_IN_CREATOR_BINPATH) {
    DEFINES += SEARCH_PUPPET_IN_CREATOR_BINPATH
    message("Search puppet in qtcreator bin path!")
}
