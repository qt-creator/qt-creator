QT += quick quickwidgets

QTC_LIB_DEPENDS += tracing
include(../../qttest.pri)

SOURCES += \
    tst_flamegraphview.cpp

HEADERS += \
    testflamegraphmodel.h

RESOURCES += \
    flamegraphview.qrc
