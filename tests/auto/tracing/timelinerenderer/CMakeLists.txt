add_qtc_test(tst_tracing_timelinerenderer
  EXCLUDE_FROM_PRECHECK
  DEPENDS Tracing Qt::Gui Qt::Quick
  SOURCES tst_timelinerenderer.cpp
)
