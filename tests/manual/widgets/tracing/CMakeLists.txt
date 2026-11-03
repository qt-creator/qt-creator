add_qtc_test(tst_manual_widgets_tracing
  MANUALTEST
  CONDITION TARGET Tracing AND TARGET Qt::CanvasPainter AND NOT EMSCRIPTEN
  DEPENDS Tracing Utils Core Qt::Gui
  SOURCES
    tst_manual_widgets_tracing.cpp
    ../common/themeselector.cpp ../common/themeselector.h
    ../common/themes.qrc
)
