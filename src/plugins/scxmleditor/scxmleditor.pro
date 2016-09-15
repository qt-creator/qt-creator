include(../../qtcreatorplugin.pri)

INCLUDEPATH += $$PWD

HEADERS += \
    scxmlcontext.h \
    scxmleditor_global.h \
    scxmleditorconstants.h \
    scxmleditordata.h \
    scxmleditordocument.h \
    scxmleditorfactory.h \
    scxmleditorplugin.h \
    scxmleditorstack.h \
    scxmltexteditor.h

SOURCES += \
    scxmlcontext.cpp \
    scxmleditordata.cpp \
    scxmleditordocument.cpp \
    scxmleditorfactory.cpp \
    scxmleditorplugin.cpp \
    scxmleditorstack.cpp \
    scxmltexteditor.cpp

RESOURCES += \
    resources.qrc

include(plugin_interface/plugin_interface.pri)
include(common/common.pri)
include(outputpane/outputpane.pri)
