QT += quick quickwidgets

QTC_LIB_DEPENDS += tracing
include(../../qttest.pri)

SOURCES += \
    tst_flamegraphview.cpp

RESOURCES += \
    flamegraphview.qrc
