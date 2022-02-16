add_qtc_test(tst_manual_fakevim
  MANUALTEST
  DEPENDS Utils
  INCLUDES "${PROJECT_SOURCE_PATH}/src/plugins/fakevim"
  DEFINES FAKEVIM_STANDALONE
  SOURCES
    main.cpp
)

extend_qtc_test(tst_manual_fakevim
  SOURCES_PREFIX "${PROJECT_SOURCE_DIR}/src/plugins/fakevim/"
  SOURCES
    fakevimactions.cpp fakevimactions.h
    fakevimhandler.cpp fakevimhandler.h
)
