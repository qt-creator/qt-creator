add_qtc_test(tst_tracing_timelinedetails
  CONDITION TARGET Tracing
  NEEDS_GUI
  DEPENDS Tracing Utils Qt::Widgets
  SOURCES tst_timelinedetailswidget.cpp
)
