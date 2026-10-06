add_qtc_plugin(FakeVim
  DEPENDS McpServerLib
  PLUGIN_DEPENDS Core TextEditor
  PLUGIN_TEST_DEPENDS CppEditor
  SOURCES ${TEST_SOURCES}
    fakevimactions.cpp fakevimactions.h
    fakevimhandler.cpp fakevimhandler.h
    fakevimplugin.cpp
    fakevimtr.h
    mcpsupport.cpp mcpsupport.h
)

extend_qtc_plugin(FakeVim
  CONDITION WITH_TESTS
  SOURCES fakevim_test.cpp fakevim_test.h
)

qtc_add_resources(FakeVim "images"
  PREFIX "/fakevim"
  FILES
    images/settingscategory_fakevim.png
    images/settingscategory_fakevim@2x.png
)
