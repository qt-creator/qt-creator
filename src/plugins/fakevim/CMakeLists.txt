add_qtc_plugin(FakeVim
  DEPENDS McpServerLib
  PLUGIN_DEPENDS Core TextEditor
  PLUGIN_TEST_DEPENDS CppEditor VcsBase
  SOURCES ${TEST_SOURCES}
    fakevim.qrc
    fakevimactions.cpp fakevimactions.h
    fakevimhandler.cpp fakevimhandler.h
    fakevimplugin.cpp
    fakevimtr.h
    mcpsupport.cpp mcpsupport.h
)

extend_qtc_plugin(FakeVim
  CONDITION WITH_TESTS
  DEPENDS VcsBase
  SOURCES fakevim_test.cpp fakevim_test.h
)
