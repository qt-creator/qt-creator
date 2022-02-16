add_qtc_test(tst_manual_pluginview
  MANUALTEST
  DEPENDS ExtensionSystem Utils
  SOURCES
    plugindialog.cpp plugindialog.h
)

add_subdirectory(plugins/plugin1)
add_subdirectory(plugins/plugin2)
add_subdirectory(plugins/plugin3)
