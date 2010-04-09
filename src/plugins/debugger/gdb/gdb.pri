HEADERS += \
    $$PWD/gdbmi.h \
    $$PWD/gdbengine.h \
    $$PWD/gdboptionspage.h \
    $$PWD/gdbchooserwidget.h \
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
    $$PWD/classicgdbengine.cpp \
    $$PWD/pythongdbengine.cpp \
    $$PWD/gdboptionspage.cpp \
    $$PWD/gdbchooserwidget.cpp \
    $$PWD/abstractgdbadapter.cpp \
    $$PWD/attachgdbadapter.cpp \
    $$PWD/coregdbadapter.cpp \
    $$PWD/plaingdbadapter.cpp \
    $$PWD/termgdbadapter.cpp \
    $$PWD/remotegdbadapter.cpp \
    $$PWD/trkgdbadapter.cpp \
    $$PWD/s60debuggerbluetoothstarter.cpp

FORMS +=  $$PWD/gdboptionspage.ui

RESOURCES += $$PWD/gdb.qrc
