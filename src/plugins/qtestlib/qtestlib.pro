TEMPLATE = lib
TARGET   = QTestLibPlugin
QT      += xml

include(../../qtcreatorplugin.pri)
include(../../plugins/coreplugin/coreplugin.pri)

SOURCES += qtestlibplugin.cpp
HEADERS += qtestlibplugin.h
RESOURCES += qtestlib.qrc

LIBS += -lProjectExplorer \
        -lQuickOpen \
        -lUtils
