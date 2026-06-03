add_qtc_test(tst_tracing_flamegraph
  NEEDS_GUI
  DEPENDS Tracing Utils Qt::Gui
  SOURCES tst_flamegraphwidget.cpp
)
