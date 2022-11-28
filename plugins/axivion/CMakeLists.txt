find_package(QtCreator COMPONENTS Core REQUIRED)
find_package(Qt5 COMPONENTS Network Widgets REQUIRED)

find_package(QtCreatorLicenseChecker QUIET)
if (TARGET QtCreator::LicenseChecker)
  set(LICENSECHECKER_DEPENDS QtCreator::LicenseChecker)
endif()

add_qtc_plugin(Axivion
  PLUGIN_DEPENDS
    QtCreator::Core QtCreator::ProjectExplorer
    ${LICENSECHECKER_DEPENDS}
  DEPENDS Qt5::Network Qt5::Widgets QtCreator::ExtensionSystem QtCreator::Utils
  SOURCES
    axivion.qrc
    axivionoutputpane.cpp axivionoutputpane.h
    axivionplugin.cpp axivionplugin.h
    axivionsettings.cpp axivionsettings.h
    axivionsettingspage.cpp axivionsettingspage.h
    axiviontr.h
)
