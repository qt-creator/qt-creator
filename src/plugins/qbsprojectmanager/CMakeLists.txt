find_package(Qbs)

add_qtc_plugin(QbsProjectManager
  CONDITION TARGET Qbs::QbsCore
  DEPENDS Qbs::QbsCore Qt5::Widgets qmljs
  DEFINES
    QBS_INSTALL_DIR="${QBS_INSTALL_DIR}"
    IDE_LIBRARY_BASENAME="${IDE_LIBRARY_BASE_PATH}"
  PLUGIN_DEPENDS Core ProjectExplorer CppTools QtSupport QmlJSTools
  SOURCES
    customqbspropertiesdialog.cpp customqbspropertiesdialog.h customqbspropertiesdialog.ui
    defaultpropertyprovider.cpp defaultpropertyprovider.h
    propertyprovider.h
    qbsbuildconfiguration.cpp qbsbuildconfiguration.h
    qbsbuildstep.cpp qbsbuildstep.h
    qbscleanstep.cpp qbscleanstep.h
    qbscleanstepconfigwidget.ui
    qbsinstallstep.cpp qbsinstallstep.h
    qbskitinformation.cpp qbskitinformation.h
    qbslogsink.cpp qbslogsink.h
    qbsnodes.cpp qbsnodes.h
    qbsnodetreebuilder.cpp qbsnodetreebuilder.h
    qbsparser.cpp qbsparser.h
    qbspmlogging.cpp qbspmlogging.h
    qbsprofilessettingspage.cpp qbsprofilessettingspage.h
    qbsprofilessettingswidget.ui
    qbsproject.cpp qbsproject.h
    qbsprojectimporter.cpp qbsprojectimporter.h
    qbsprojectmanager.cpp qbsprojectmanager.h qbsprojectmanager.qrc
    qbsprojectmanager_global.h
    qbsprojectmanagerconstants.h
    qbsprojectmanagerplugin.cpp qbsprojectmanagerplugin.h
    qbsprojectmanagersettings.cpp qbsprojectmanagersettings.h
    qbsprojectparser.cpp qbsprojectparser.h
)
