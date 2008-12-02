TEMPLATE = lib
TARGET = ExtensionSystem
QT += xml
DEFINES += EXTENSIONSYSTEM_LIBRARY
include(../../qworkbenchlibrary.pri)
include(extensionsystem_dependencies.pri)

DEFINES += IDE_TEST_DIR=\\\"$$IDE_SOURCE_TREE\\\"

HEADERS += pluginerrorview.h \
    plugindetailsview.h \
    iplugin.h \
    iplugin_p.h \
    extensionsystem_global.h \
    pluginmanager.h \
    pluginmanager_p.h \
    pluginspec.h \
    pluginspec_p.h \
    pluginview.h \
    pluginview_p.h \
    optionsparser.h
SOURCES += pluginerrorview.cpp \
    plugindetailsview.cpp \
    iplugin.cpp \
    pluginmanager.cpp \
    pluginspec.cpp \
    pluginview.cpp \
    optionsparser.cpp
FORMS += pluginview.ui \
    pluginerrorview.ui \
    plugindetailsview.ui
RESOURCES += pluginview.qrc
