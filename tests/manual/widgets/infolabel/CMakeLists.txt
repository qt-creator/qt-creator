add_qtc_test(tst_manual_widgets_infolabel
  MANUALTEST
  DEPENDS Qt::Gui Utils Core
  SOURCES
    tst_manual_widgets_infolabel.cpp
    ../common/themeselector.cpp ../common/themeselector.h
    ../common/themes.qrc
)
