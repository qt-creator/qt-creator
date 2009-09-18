include(../../../shared/trk/trk.pri)

HEADERS += \
    $$PWD/abstractgdbadapter.h \
    $$PWD/gdbmi.h \
    $$PWD/gdbengine.h \
    $$PWD/gdboptionspage.h \
    $$PWD/trkgdbadapter.h \
    $$PWD/trkoptions.h \
    $$PWD/trkoptionswidget.h \
    $$PWD/trkoptionspage.h

SOURCES += \
    $$PWD/gdbmi.cpp \
    $$PWD/gdbengine.cpp \
    $$PWD/gdboptionspage.cpp \
    $$PWD/trkgdbadapter.cpp \
    $$PWD/trkoptions.cpp \
    $$PWD/trkoptionswidget.cpp \
    $$PWD/trkoptionspage.cpp

FORMS +=  $$PWD/gdboptionspage.ui \
$$PWD/trkoptionswidget.ui

RESOURCES += $$PWD/gdb.qrc
