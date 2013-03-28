DEFINES += EXTENSIONSYSTEM_LIBRARY
include(../../qtcreatorlibrary.pri)

unix:!macx:!freebsd*:LIBS += -ldl

!isEmpty(vcproj) {
    DEFINES += IDE_TEST_DIR=\"$$IDE_SOURCE_TREE\"
} else {
    DEFINES += IDE_TEST_DIR=\\\"$$IDE_SOURCE_TREE\\\"
}

HEADERS += pluginerrorview.h \
    plugindetailsview.h \
    invoker.h \
    iplugin.h \
    iplugin_p.h \
    extensionsystem_global.h \
    pluginmanager.h \
    pluginmanager_p.h \
    pluginspec.h \
    pluginspec_p.h \
    pluginview.h \
    optionsparser.h \
    plugincollection.h \
    pluginerroroverview.h
SOURCES += pluginerrorview.cpp \
    plugindetailsview.cpp \
    invoker.cpp \
    iplugin.cpp \
    pluginmanager.cpp \
    pluginspec.cpp \
    pluginview.cpp \
    optionsparser.cpp \
    plugincollection.cpp \
    pluginerroroverview.cpp
FORMS += pluginview.ui \
    pluginerrorview.ui \
    plugindetailsview.ui \
    pluginerroroverview.ui
RESOURCES += pluginview.qrc



