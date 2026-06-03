add_qtc_test(tst_tracing_flamegraphview
  NEEDS_GUI
  EXCLUDE_FROM_PRECHECK
  DEPENDS Tracing Utils Qt::Gui
  SOURCES tst_flamegraphwidgetview.cpp
)
