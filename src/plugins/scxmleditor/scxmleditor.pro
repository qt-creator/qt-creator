include(../../qtcreatorplugin.pri)

INCLUDEPATH += $$PWD

HEADERS += \
    scxmleditor_global.h \
    scxmleditorconstants.h \
    scxmleditordata.h \
    scxmleditordocument.h \
    scxmleditorfactory.h \
    scxmleditorplugin.h \
    scxmleditorstack.h \
    scxmltexteditor.h

SOURCES += \
    scxmleditordata.cpp \
    scxmleditordocument.cpp \
    scxmleditorfactory.cpp \
    scxmleditorplugin.cpp \
    scxmleditorstack.cpp \
    scxmltexteditor.cpp

include(plugin_interface/plugin_interface.pri)
include(common/common.pri)
include(outputpane/outputpane.pri)
