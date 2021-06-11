add_qtc_test(tst_manual_widgets_manhattanstyle
  DEPENDS Qt5::Gui Utils Core
  SOURCES
    tst_manual_widgets_manhattanstyle.cpp
    ../common/themeselector.cpp ../common/themeselector.h
    ../common/themes.qrc
)
