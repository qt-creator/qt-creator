if (WITH_TESTS)
  set(TST_COMPONENT CppEditor CppTools)
endif()

add_qtc_plugin(FakeVim
  PLUGIN_DEPENDS Core TextEditor ${TST_COMPONENT}
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
