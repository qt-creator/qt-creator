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

if (MSVC)
  # Prevent fatal error C1128: both fakevimhandler.cpp and fakevim_test.cpp
  # hold more sections than the object format takes by default.
  target_compile_options(FakeVim PRIVATE /bigobj)
endif()
if (MINGW)
  target_compile_options(FakeVim PRIVATE "-Wa,-mbig-obj")
endif()
