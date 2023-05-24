find_package(QtCreator COMPONENTS Core REQUIRED)
find_package(Qt6 COMPONENTS Network Widgets REQUIRED)

find_package(QtCreatorLicenseChecker QUIET)
if (TARGET QtCreator::LicenseChecker)
  set(LICENSECHECKER_DEPENDS QtCreator::LicenseChecker)
endif()

add_qtc_plugin(Axivion
  PLUGIN_DEPENDS
    QtCreator::Core QtCreator::ProjectExplorer QtCreator::TextEditor
    ${LICENSECHECKER_DEPENDS}
  DEPENDS Qt::Network Qt::Widgets QtCreator::ExtensionSystem QtCreator::Utils
  SOURCES
    axivion.qrc
    axivionoutputpane.cpp axivionoutputpane.h
    axivionplugin.cpp axivionplugin.h
    axivionprojectsettings.h axivionprojectsettings.cpp
    axivionquery.h axivionquery.cpp
    axivionresultparser.h axivionresultparser.cpp
    axivionsettings.cpp axivionsettings.h
    axivionsettingspage.cpp axivionsettingspage.h
    axiviontr.h
)
