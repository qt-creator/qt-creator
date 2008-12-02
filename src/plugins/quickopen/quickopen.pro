TEMPLATE = lib
TARGET = QuickOpen
DEFINES += QUICKOPEN_LIBRARY
include(../../qworkbenchplugin.pri)
include(quickopen_dependencies.pri)
HEADERS += quickopenplugin.h \
    quickopentoolwindow.h \
    quickopenfiltersfilter.h \
    settingspage.h \
    iquickopenfilter.h \
    opendocumentsfilter.h \
    filesystemfilter.h \
    quickopenconstants.h \
    directoryfilter.h \
    quickopenmanager.h \
    basefilefilter.h \
    quickopen_global.h
SOURCES += quickopenplugin.cpp \
    quickopentoolwindow.cpp \
    quickopenfiltersfilter.cpp \
    opendocumentsfilter.cpp \
    filesystemfilter.cpp \
    settingspage.cpp \
    directoryfilter.cpp \
    quickopenmanager.cpp \
    basefilefilter.cpp \
    iquickopenfilter.cpp
FORMS += settingspage.ui \
    filesystemfilter.ui \
    directoryfilter.ui
RESOURCES += quickopen.qrc
