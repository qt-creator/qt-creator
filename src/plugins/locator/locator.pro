TEMPLATE = lib
TARGET = Locator
DEFINES += LOCATOR_LIBRARY QT_NO_CAST_FROM_ASCII
include(../../qtcreatorplugin.pri)
include(locator_dependencies.pri)
HEADERS += locatorplugin.h \
    commandlocator.h \
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
    locator_global.h \
    executefilter.h
SOURCES += locatorplugin.cpp \
    commandlocator.cpp \
    locatorwidget.cpp \
    locatorfiltersfilter.cpp \
    opendocumentsfilter.cpp \
    filesystemfilter.cpp \
    settingspage.cpp \
    directoryfilter.cpp \
    locatormanager.cpp \
    basefilefilter.cpp \
    ilocatorfilter.cpp \
    executefilter.cpp

FORMS += settingspage.ui \
    filesystemfilter.ui \
    directoryfilter.ui
RESOURCES += locator.qrc
