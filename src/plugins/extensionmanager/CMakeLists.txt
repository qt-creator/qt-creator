add_qtc_plugin(ExtensionManager
  PLUGIN_DEPENDS Core
  SOURCES
    extensionmanager.qrc
    extensionmanagerconstants.h
    extensionmanagerplugin.cpp
    extensionmanagertr.h
    extensionmanagersettings.cpp
    extensionmanagersettings.h
    extensionmanagerwidget.cpp
    extensionmanagerwidget.h
    extensionsbrowser.cpp
    extensionsbrowser.h
    extensionsmodel.cpp
    extensionsmodel.h
)

extend_qtc_plugin(ExtensionManager
  CONDITION WITH_TESTS
  SOURCES
    extensionmanager_test.cpp
    extensionmanager_test.h
    extensionmanager_test.qrc
)
