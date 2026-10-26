add_qtc_test(tst_tracing_timelineoverviewwidget
  CONDITION TARGET Tracing
  NEEDS_GUI
  DEPENDS Tracing Utils Qt::Widgets
  SOURCES tst_timelineoverviewwidget.cpp
)
