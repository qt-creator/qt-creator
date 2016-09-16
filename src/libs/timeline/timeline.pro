QT += qml quick
DEFINES += TIMELINE_LIBRARY

include(../../qtcreatorlibrary.pri)

SOURCES += \
    $$PWD/timelinemodel.cpp \
    $$PWD/timelinemodelaggregator.cpp \
    $$PWD/timelinerenderer.cpp \
    $$PWD/timelinezoomcontrol.cpp \
    $$PWD/timelineitemsrenderpass.cpp \
    $$PWD/timelineselectionrenderpass.cpp \
    $$PWD/timelinenotesrenderpass.cpp \
    $$PWD/timelinerenderpass.cpp \
    $$PWD/timelinerenderstate.cpp \
    $$PWD/timelinenotesmodel.cpp \
    $$PWD/timelineabstractrenderer.cpp \
    $$PWD/timelineoverviewrenderer.cpp \
    $$PWD/timelinetheme.cpp \
    $$PWD/timelineformattime.cpp

HEADERS += \
    $$PWD/timeline_global.h \
    $$PWD/timelinemodel.h \
    $$PWD/timelinemodel_p.h \
    $$PWD/timelinemodelaggregator.h \
    $$PWD/timelinerenderer.h \
    $$PWD/timelinezoomcontrol.h \
    $$PWD/timelineitemsrenderpass.h \
    $$PWD/timelineselectionrenderpass.h \
    $$PWD/timelinenotesrenderpass.h \
    $$PWD/timelinerenderpass.h \
    $$PWD/timelinerenderstate.h \
    $$PWD/timelinenotesmodel.h \
    $$PWD/timelinenotesmodel_p.h \
    $$PWD/timelinerenderer_p.h \
    $$PWD/timelinerenderstate_p.h \
    $$PWD/timelineabstractrenderer.h \
    $$PWD/timelineabstractrenderer_p.h \
    $$PWD/timelineoverviewrenderer_p.h \
    $$PWD/timelineoverviewrenderer.h \
    $$PWD/timelinetheme.h \
    $$PWD/timelineformattime.h

RESOURCES += \
    $$PWD/qml/timeline.qrc

DISTFILES += README

equals(TEST, 1) {
    SOURCES += runscenegraphtest.cpp
    HEADERS += runscenegraphtest.h
}
