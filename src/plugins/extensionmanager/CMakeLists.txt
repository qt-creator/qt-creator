add_qtc_plugin(ExtensionManager
  PLUGIN_DEPENDS Core
  SOURCES
    extensionmanager.qrc
    extensionmanager_global.h
    extensionmanagerconstants.h
    extensionmanagerlegalnotice.cpp
    extensionmanagerlegalnotice.h
    extensionmanagerplugin.cpp
    extensionmanagersettings.cpp
    extensionmanagersettings.h
    extensionmanagertr.h
    extensionmanagerwidget.cpp
    extensionmanagerwidget.h
    extensionsbrowser.cpp
    extensionsbrowser.h
    extensionsmodel.cpp
    extensionsmodel.h
    remotespec.cpp
    remotespec.h
)

if (WITH_TESTS)
  qt_add_resources(ExtensionManager extensionmanager_test_qrc
    PREFIX "/extensionmanager"
    FILES
      testdata/defaultdata/registry/3dlights.vale/extension.json
      testdata/defaultdata/registry/cristianadam.dracula/extension.json
      testdata/defaultdata/registry/theqtcompany.cpp/pack.json
  )
endif()

extend_qtc_plugin(ExtensionManager
  CONDITION WITH_TESTS
  SOURCES
    extensionmanager_test.cpp
    extensionmanager_test.h
)
