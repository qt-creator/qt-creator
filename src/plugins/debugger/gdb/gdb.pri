HEADERS += \
    $$PWD/gdbprocessbase.h \
    $$PWD/gdbmi.h \
    $$PWD/gdbengine.h \
    $$PWD/gdboptionspage.h \
    $$PWD/callback.h \
    $$PWD/trkutils.h \
    $$PWD/trkclient.h \
    $$PWD/symbianadapter.h \
    #$$PWD/gdboptionspage.h \

SOURCES += \
    $$PWD/gdbmi.cpp \
    $$PWD/gdbengine.cpp \
    $$PWD/gdboptionspage.cpp \
    $$PWD/trkutils.cpp \
    $$PWD/trkclient.cpp \
    $$PWD/symbianadapter.cpp \
    $$PWD/symbianengine.cpp \
    #$$PWD/gdboptionspage.cpp \

FORMS +=  $$PWD/gdboptionspage.ui

RESOURCES += $$PWD/gdb.qrc
