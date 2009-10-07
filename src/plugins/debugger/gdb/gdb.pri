include(../../../shared/trk/trk.pri)

!isEmpty(SUPPORT_QT_S60) {
    message("Adding experimental support for Qt/S60 applications.")
    DEFINES += QTCREATOR_WITH_S60
}

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

FORMS +=  $$PWD/gdboptionspage.ui \
$$PWD/trkoptionswidget.ui

RESOURCES += $$PWD/gdb.qrc
