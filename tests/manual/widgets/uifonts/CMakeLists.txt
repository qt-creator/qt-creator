add_qtc_test(tst_manual_widgets_uifonts
  MANUALTEST
  DEPENDS Qt::Gui Utils
  SOURCES
    tst_manual_widgets_uifonts.cpp
)

set(FONTS_BASE "${QtCreator_SOURCE_DIR}/src/share/3rdparty/studiofonts/")
qt_add_resources(tst_manual_widgets_uifonts
  tst_manual_widgets_uifonts_rcc
  BASE ${FONTS_BASE}
  PREFIX "studiofonts"
  FILES "${FONTS_BASE}/TitilliumWeb-Regular.ttf"
)
