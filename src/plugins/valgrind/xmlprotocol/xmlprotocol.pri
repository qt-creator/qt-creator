QT *= network

HEADERS += \
    $$PWD/frame.h \
    $$PWD/parser.h \
    $$PWD/error.h \
    $$PWD/status.h \
    $$PWD/suppression.h \
    $$PWD/threadedparser.h \
    $$PWD/announcethread.h \
    $$PWD/stack.h \
    $$PWD/errorlistmodel.h \
    $$PWD/stackmodel.h \
    $$PWD/modelhelpers.h

SOURCES += \
    $$PWD/error.cpp \
    $$PWD/frame.cpp \
    $$PWD/parser.cpp \
    $$PWD/status.cpp \
    $$PWD/suppression.cpp \
    $$PWD/threadedparser.cpp \
    $$PWD/announcethread.cpp \
    $$PWD/stack.cpp \
    $$PWD/errorlistmodel.cpp \
    $$PWD/stackmodel.cpp \
    $$PWD/modelhelpers.cpp
