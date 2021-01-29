add_qtc_executable(tst_manual_widgets_manhattanstyle
  DEPENDS Qt5::Gui Utils Core
  SOURCES
    tst_manual_widgets_manhattanstyle.cpp
    ../common/themeselector.cpp ../common/themeselector.h
    ../common/themes.qrc
  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
