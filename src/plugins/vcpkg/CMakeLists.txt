add_qtc_plugin(Vcpkg
  PLUGIN_DEPENDS Core ProjectExplorer
  SOURCES
    vcpkgconstants.h
    vcpkgmanifesteditor.cpp vcpkgmanifesteditor.h
    vcpkgplugin.cpp
    vcpkgsearch.cpp vcpkgsearch.h
    vcpkgsettings.cpp vcpkgsettings.h
    vcpkgtr.h
)

extend_qtc_plugin(Vcpkg
  CONDITION WITH_TESTS
  SOURCES
    vcpkg_test.cpp vcpkg_test.h
)

qtc_add_resources(Vcpkg "resources"
  PREFIX "/vcpkg"
  FILES
    images/vcpkgicon.png
    images/vcpkgicon@2x.png
    wizards/manifest/vcpkg.json.tpl
    wizards/manifest/wizard.json
)
