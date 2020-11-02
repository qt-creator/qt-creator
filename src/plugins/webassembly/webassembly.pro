include(../../qtcreatorplugin.pri)

HEADERS += \
    webassembly_global.h \
    webassemblyconstants.h \
    webassemblydevice.h \
    webassemblyemsdk.h \
    webassemblyoptionspage.h \
    webassemblyplugin.h \
    webassemblyqtversion.h \
    webassemblyrunconfigurationaspects.h \
    webassemblyrunconfiguration.h \
    webassemblytoolchain.h

SOURCES += \
    webassemblydevice.cpp \
    webassemblyemsdk.cpp \
    webassemblyoptionspage.cpp \
    webassemblyplugin.cpp \
    webassemblyqtversion.cpp \
    webassemblyrunconfigurationaspects.cpp \
    webassemblyrunconfiguration.cpp \
    webassemblytoolchain.cpp

RESOURCES += \
    webassembly.qrc
