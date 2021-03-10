add_qtc_plugin(FakeVim
  CONDITION Qt5_VERSION VERSION_LESS 6.0.0
  PLUGIN_DEPENDS Core TextEditor
  PLUGIN_TEST_DEPENDS CppEditor CppTools
  SOURCES ${TEST_SOURCES}
    fakevim.qrc
    fakevimactions.cpp fakevimactions.h
    fakevimhandler.cpp fakevimhandler.h
    fakevimoptions.ui
    fakevimplugin.cpp fakevimplugin.h
    fakevimtr.h
)

extend_qtc_plugin(FakeVim
  CONDITION WITH_TESTS
  SOURCES fakevim_test.cpp
)
