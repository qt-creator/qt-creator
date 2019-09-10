if (WITH_TESTS)
  set(TEST_SOURCES
    runscenegraphtest.cpp runscenegraphtest.h
  )
endif()

add_qtc_library(Tracing
  DEPENDS Utils Qt5::Qml Qt5::Quick
  PUBLIC_DEPENDS Qt5::Widgets
  SOURCES ${TEST_SOURCES}
    flamegraph.cpp flamegraph.h
    flamegraphattached.h
    safecastable.h
    qml/tracing.qrc
    timelineabstractrenderer.cpp timelineabstractrenderer.h timelineabstractrenderer_p.h
    timelineformattime.cpp timelineformattime.h
    timelineitemsrenderpass.cpp timelineitemsrenderpass.h
    timelinemodel.cpp timelinemodel.h timelinemodel_p.h
    timelinemodelaggregator.cpp timelinemodelaggregator.h
    timelinenotesmodel.cpp timelinenotesmodel.h timelinenotesmodel_p.h
    timelinenotesrenderpass.cpp timelinenotesrenderpass.h
    timelineoverviewrenderer.cpp timelineoverviewrenderer.h timelineoverviewrenderer_p.h
    timelinerenderer.cpp timelinerenderer.h timelinerenderer_p.h
    timelinerenderpass.cpp timelinerenderpass.h
    timelinerenderstate.cpp timelinerenderstate.h timelinerenderstate_p.h
    timelineselectionrenderpass.cpp timelineselectionrenderpass.h
    timelinetheme.cpp timelinetheme.h
    timelinetracefile.cpp timelinetracefile.h
    timelinetracemanager.cpp timelinetracemanager.h
    timelinezoomcontrol.cpp timelinezoomcontrol.h
    traceevent.h
    traceeventtype.h
    tracestashfile.h
    tracing_global.h
)
