add_qtc_test(tst_fakevim
  DEPENDS Utils Qt::Widgets
  INCLUDES "${PROJECT_SOURCE_DIR}/src/plugins/fakevim"
  DEFINES FAKEVIM_STANDALONE
  SOURCES
    tst_fakevim.cpp
)

extend_qtc_test(tst_fakevim
  SOURCES_PREFIX "${PROJECT_SOURCE_DIR}/src/plugins/fakevim/"
  SOURCES
    fakevimactions.cpp fakevimactions.h
    fakevimhandler.cpp fakevimhandler.h
)
