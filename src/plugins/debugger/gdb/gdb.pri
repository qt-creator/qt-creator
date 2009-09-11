HEADERS += \
    $$PWD/abstractgdbadapter.h \
    $$PWD/gdbmi.h \
    $$PWD/gdbengine.h \
    $$PWD/gdboptionspage.h \
    $$PWD/callback.h \
    $$PWD/trkutils.h \
    $$PWD/trkclient.h \
    $$PWD/trkgdbadapter.h \
    #$$PWD/gdboptionspage.h \

SOURCES += \
    $$PWD/gdbmi.cpp \
    $$PWD/gdbengine.cpp \
    $$PWD/gdboptionspage.cpp \
    $$PWD/trkutils.cpp \
    $$PWD/trkclient.cpp \
    $$PWD/trkgdbadapter.cpp \
    #$$PWD/gdboptionspage.cpp \

FORMS +=  $$PWD/gdboptionspage.ui

RESOURCES += $$PWD/gdb.qrc
