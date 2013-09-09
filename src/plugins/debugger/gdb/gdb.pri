HEADERS += \
    $$PWD/gdbengine.h \
    $$PWD/gdboptionspage.h \
    $$PWD/attachgdbadapter.h \
    $$PWD/coregdbadapter.h \
    $$PWD/termgdbadapter.h \
    $$PWD/remotegdbserveradapter.h \
    $$PWD/gdbplainengine.h \
    $$PWD/localgdbprocess.h \
    $$PWD/abstractgdbprocess.h \
    $$PWD/startgdbserverdialog.h

SOURCES += \
    $$PWD/gdbengine.cpp \
    $$PWD/classicgdbengine.cpp \
    $$PWD/pythongdbengine.cpp \
    $$PWD/gdboptionspage.cpp \
    $$PWD/attachgdbadapter.cpp \
    $$PWD/coregdbadapter.cpp \
    $$PWD/termgdbadapter.cpp \
    $$PWD/remotegdbserveradapter.cpp \
    $$PWD/abstractgdbprocess.cpp \
    $$PWD/localgdbprocess.cpp \
    $$PWD/gdbplainengine.cpp \
    $$PWD/startgdbserverdialog.cpp

RESOURCES += $$PWD/gdb.qrc
