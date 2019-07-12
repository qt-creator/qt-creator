include(../../qtcreatorplugin.pri)

HEADERS += \
    webassembly_global.h \
    webassemblyconstants.h \
    webassemblydevice.h \
    webassemblyplugin.h \
    webassemblyqtversion.h \
    webassemblyrunconfigurationaspects.h \
    webassemblyrunconfiguration.h \
    webassemblytoolchain.h

SOURCES += \
    webassemblydevice.cpp \
    webassemblyplugin.cpp \
    webassemblyqtversion.cpp \
    webassemblyrunconfigurationaspects.cpp \
    webassemblyrunconfiguration.cpp \
    webassemblytoolchain.cpp

RESOURCES += \
    webassembly.qrc
