add_qtc_test(tst_tracing_timelinecontentwidget
  CONDITION TARGET Tracing
  NEEDS_GUI
  DEPENDS Tracing Utils Qt::Widgets
  SOURCES tst_timelinecontentwidget.cpp
)
