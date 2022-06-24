add_qtc_test(tst_tracing_timelinerenderer
  EXCLUDE_FROM_PRECHECK
  DEPENDS Tracing Qt5::Gui Qt5::Quick
  SOURCES tst_timelinerenderer.cpp
)
