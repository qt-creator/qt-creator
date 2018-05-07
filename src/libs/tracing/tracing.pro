QT += qml quick
DEFINES += TRACING_LIBRARY

include(../../qtcreatorlibrary.pri)

SOURCES += \
    $$PWD/flamegraph.cpp \
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
    $$PWD/timelineformattime.cpp \
    $$PWD/timelinetracefile.cpp \
    $$PWD/timelinetracemanager.cpp

HEADERS += \
    $$PWD/flamegraph.h \
    $$PWD/flamegraphattached.h \
    $$PWD/safecastable.h \
    $$PWD/tracing_global.h \
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
    $$PWD/timelineformattime.h \
    $$PWD/timelinetracefile.h \
    $$PWD/timelinetracemanager.h \
    $$PWD/traceevent.h \
    $$PWD/traceeventtype.h \
    $$PWD/tracestashfile.h

RESOURCES += \
    $$PWD/qml/tracing.qrc

DISTFILES += README

equals(TEST, 1) {
    SOURCES += runscenegraphtest.cpp
    HEADERS += runscenegraphtest.h
}
