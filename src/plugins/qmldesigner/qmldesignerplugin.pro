QT += quickwidgets core-private
CONFIG += exceptions

INCLUDEPATH += $$PWD
INCLUDEPATH += $$PWD/components

unix:!openbsd:!osx: LIBS += -lrt # posix shared memory

include(../../qtcreatorplugin.pri)

include(qmldesignerplugin.pri)
include(designercore/designercore-lib.pri)
include(components/componentcore/componentcore.pri)
include(components/integration/integration.pri)
include(components/propertyeditor/propertyeditor.pri)
include(components/edit3d/edit3d.pri)
include(components/formeditor/formeditor.pri)
include(components/texteditor/texteditor.pri)
include(components/itemlibrary/itemlibrary.pri)
include(components/navigator/navigator.pri)
include(components/stateseditor/stateseditor.pri)
include(components/resources/resources.pri)
include(components/debugview/debugview.pri)
include(components/importmanager/importmanager.pri)
include(components/sourcetool/sourcetool.pri)
include(components/colortool/colortool.pri)
include(components/texttool/texttool.pri)
include(components/pathtool/pathtool.pri)
include(components/timelineeditor/timelineeditor.pri)
include(components/connectioneditor/connectioneditor.pri)
include(components/curveeditor/curveeditor.pri)
include(components/bindingeditor/bindingeditor.pri)
include(components/annotationeditor/annotationeditor.pri)
include(components/richtexteditor/richtexteditor.pri)
include(components/transitioneditor/transitioneditor.pri)
include(components/listmodeleditor/listmodeleditor.pri)

BUILD_PUPPET_IN_CREATOR_BINPATH = $$(BUILD_PUPPET_IN_CREATOR_BINPATH)
!isEmpty(BUILD_PUPPET_IN_CREATOR_BINPATH) {
    DEFINES += SEARCH_PUPPET_IN_CREATOR_BINPATH
    message("Search puppet in qtcreator bin path!")
}
