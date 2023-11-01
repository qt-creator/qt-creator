add_qtc_plugin(Vcpkg
  PLUGIN_DEPENDS Core ProjectExplorer
  SOURCES
    vcpkg.qrc
    vcpkgconstants.h
    vcpkgmanifesteditor.cpp vcpkgmanifesteditor.h
    vcpkgplugin.cpp
    vcpkgsearch.cpp vcpkgsearch.h
    vcpkgsettings.cpp vcpkgsettings.h
)

extend_qtc_plugin(Vcpkg
  CONDITION WITH_TESTS
  SOURCES
    vcpkg_test.cpp vcpkg_test.h
  EXPLICIT_MOC vcpkg_test.h
)
