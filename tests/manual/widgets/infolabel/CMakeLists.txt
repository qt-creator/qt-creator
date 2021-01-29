add_qtc_executable(tst_manual_widgets_infolabel
  DEPENDS Qt5::Gui Utils Core
  SOURCES
    tst_manual_widgets_infolabel.cpp
    ../common/themeselector.cpp ../common/themeselector.h
    ../common/themes.qrc
  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
