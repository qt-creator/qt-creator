QT += qml quick
DEFINES += FLAMEGRAPH_LIBRARY

include(../../qtcreatorlibrary.pri)

SOURCES += \
    $$PWD/flamegraph.cpp

HEADERS += \
    $$PWD/flamegraph.h \
    $$PWD/flamegraph_global.h \
    $$PWD/flamegraphattached.h

RESOURCES += \
    $$PWD/qml/flamegraph.qrc

