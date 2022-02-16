add_qtc_test(tst_manual_widgets_tracing
  MANUALTEST
  CONDITION TARGET Tracing
  DEPENDS Tracing Utils Core Qt5::Gui Qt5::Quick
  SOURCES
    tst_manual_widgets_tracing.cpp
    ../common/themeselector.cpp ../common/themeselector.h
    ../common/themes.qrc
)
