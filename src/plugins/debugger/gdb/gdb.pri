include(../../../shared/trk/trk.pri)

HEADERS += \
    $$PWD/gdbmi.h \
    $$PWD/gdbengine.h \
    $$PWD/gdboptionspage.h \
    $$PWD/trkoptions.h \
    $$PWD/trkoptionswidget.h \
    $$PWD/trkoptionspage.h \
    $$PWD/abstractgdbadapter.h \
    $$PWD/attachgdbadapter.h \
    $$PWD/coregdbadapter.h \
    $$PWD/plaingdbadapter.h \
    $$PWD/termgdbadapter.h \
    $$PWD/remotegdbadapter.h \
    $$PWD/trkgdbadapter.h \
    $$PWD/s60debuggerbluetoothstarter.h

SOURCES += \
    $$PWD/gdbmi.cpp \
    $$PWD/gdbengine.cpp \
    $$PWD/gdboptionspage.cpp \
    $$PWD/trkoptions.cpp \
    $$PWD/trkoptionswidget.cpp \
    $$PWD/trkoptionspage.cpp \
    $$PWD/abstractgdbadapter.cpp \
    $$PWD/attachgdbadapter.cpp \
    $$PWD/coregdbadapter.cpp \
    $$PWD/plaingdbadapter.cpp \
    $$PWD/termgdbadapter.cpp \
    $$PWD/remotegdbadapter.cpp \
    $$PWD/trkgdbadapter.cpp \
    $$PWD/s60debuggerbluetoothstarter.cpp

FORMS +=  $$PWD/gdboptionspage.ui \
$$PWD/trkoptionswidget.ui

RESOURCES += $$PWD/gdb.qrc
