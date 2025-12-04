add_qtc_test(tst_manual_widgets_aspects
  MANUALTEST
  DEPENDS Qt::Gui Utils Core ProjectExplorer
  SOURCES
    tst_manual_widgets_aspects.cpp
    ../common/themeselector.cpp ../common/themeselector.h
    ../common/themes.qrc
)
