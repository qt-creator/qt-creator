HEADERS += \
    $$PWD/gdbengine.h \
    $$PWD/gdboptionspage.h \
    $$PWD/attachgdbadapter.h \
    $$PWD/coregdbadapter.h \
    $$PWD/localplaingdbadapter.h \
    $$PWD/termgdbadapter.h \
    $$PWD/remotegdbserveradapter.h \
    $$PWD/abstractgdbprocess.h \
    $$PWD/localgdbprocess.h \
    $$PWD/remotegdbprocess.h \
    $$PWD/remoteplaingdbadapter.h \
    $$PWD/abstractplaingdbadapter.h \
    $$PWD/startgdbserverdialog.h

SOURCES += \
    $$PWD/gdbengine.cpp \
    $$PWD/classicgdbengine.cpp \
    $$PWD/pythongdbengine.cpp \
    $$PWD/gdboptionspage.cpp \
    $$PWD/attachgdbadapter.cpp \
    $$PWD/coregdbadapter.cpp \
    $$PWD/localplaingdbadapter.cpp \
    $$PWD/termgdbadapter.cpp \
    $$PWD/remotegdbserveradapter.cpp \
    $$PWD/abstractgdbprocess.cpp \
    $$PWD/localgdbprocess.cpp \
    $$PWD/remotegdbprocess.cpp \
    $$PWD/remoteplaingdbadapter.cpp \
    $$PWD/abstractplaingdbadapter.cpp \
    $$PWD/startgdbserverdialog.cpp

RESOURCES += $$PWD/gdb.qrc
