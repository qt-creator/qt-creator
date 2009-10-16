TEMPLATE = lib
TARGET = Locator
DEFINES += LOCATOR_LIBRARY
include(../../qtcreatorplugin.pri)
include(locator_dependencies.pri)
HEADERS += locatorplugin.h \
    locatorwidget.h \
    locatorfiltersfilter.h \
    settingspage.h \
    ilocatorfilter.h \
    opendocumentsfilter.h \
    filesystemfilter.h \
    locatorconstants.h \
    directoryfilter.h \
    locatormanager.h \
    basefilefilter.h \
    locator_global.h
SOURCES += locatorplugin.cpp \
    locatorwidget.cpp \
    locatorfiltersfilter.cpp \
    opendocumentsfilter.cpp \
    filesystemfilter.cpp \
    settingspage.cpp \
    directoryfilter.cpp \
    locatormanager.cpp \
    basefilefilter.cpp \
    ilocatorfilter.cpp
FORMS += settingspage.ui \
    filesystemfilter.ui \
    directoryfilter.ui
RESOURCES += locator.qrc

OTHER_FILES += Locator.pluginspec
