include(../../shared/trk/trk.pri)

HEADERS += \
    $$PWD/abstractgdbadapter.h \
    $$PWD/gdbmi.h \
    $$PWD/gdbengine.h \
    $$PWD/gdboptionspage.h \
    $$PWD/trkgdbadapter.h \
    #$$PWD/gdboptionspage.h \

SOURCES += \
    $$PWD/gdbmi.cpp \
    $$PWD/gdbengine.cpp \
    $$PWD/gdboptionspage.cpp \
    $$PWD/trkgdbadapter.cpp

FORMS +=  $$PWD/gdboptionspage.ui

RESOURCES += $$PWD/gdb.qrc
