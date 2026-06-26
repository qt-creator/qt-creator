add_qtc_test(tst_tracing_timelineabstractrenderer
  CONDITION TARGET Tracing
  DEPENDS Tracing Qt::Gui
  SOURCES tst_trackpainterproperties.cpp
)
