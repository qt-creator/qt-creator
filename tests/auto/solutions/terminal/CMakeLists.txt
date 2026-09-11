add_qtc_test(tst_terminalsurface
  INCLUDES "${PROJECT_SOURCE_DIR}/src/libs"
  DEPENDS Qt::Gui TerminalLib
  SOURCES tst_terminalsurface.cpp
)

add_qtc_test(tst_terminalview
  INCLUDES "${PROJECT_SOURCE_DIR}/src/libs"
  DEPENDS Qt::Widgets TerminalLib
  SOURCES tst_terminalview.cpp
)

add_qtc_test(tst_sixel
  INCLUDES "${PROJECT_SOURCE_DIR}/src/libs"
  DEPENDS Qt::Gui TerminalLib
  SOURCES tst_sixel.cpp
)
