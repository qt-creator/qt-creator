add_qtc_test(tst_tracing_rangedetails
  CONDITION TARGET Tracing
  NEEDS_GUI
  DEPENDS Tracing Utils Qt::Widgets
  SOURCES tst_rangedetailswidget.cpp
)
