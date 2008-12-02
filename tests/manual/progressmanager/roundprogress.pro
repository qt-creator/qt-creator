TARGET = roundprogress
TEMPLATE = app
QT += core \
    gui
INCLUDEPATH += $$PWD/../../../src/plugins/core/progressmanager $$PWD/../../../src/libs/qtconcurrent
SOURCES += main.cpp \
    roundprogress.cpp \
    $$PWD/../../../src/plugins/core/progressmanager/progresspie.cpp \
    $$PWD/../../../src/plugins/core/progressmanager/futureprogress.cpp
HEADERS += roundprogress.h \
    $$PWD/../../../src/libs/qtconcurrent/multitask.h \
    $$PWD/../../../src/plugins/core/progressmanager/progresspie_p.h \
    $$PWD/../../../src/plugins/core/progressmanager/progresspie.h \
    $$PWD/../../../src/plugins/core/progressmanager/futureprogress.h
FORMS += roundprogress.ui
