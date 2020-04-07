add_qtc_plugin(QbsProjectManager
  DEPENDS Qt5::Qml Qt5::Widgets QmlJS
  DEFINES
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
    qbsnodes.cpp qbsnodes.h
    qbsnodetreebuilder.cpp qbsnodetreebuilder.h
    qbspmlogging.cpp qbspmlogging.h
    qbsprofilemanager.cpp qbsprofilemanager.h
    qbsprofilessettingspage.cpp qbsprofilessettingspage.h
    qbsprofilessettingswidget.ui
    qbsproject.cpp qbsproject.h
    qbsprojectimporter.cpp qbsprojectimporter.h
    qbsprojectmanager.qrc
    qbsprojectmanager_global.h
    qbsprojectmanagerconstants.h
    qbsprojectmanagerplugin.cpp qbsprojectmanagerplugin.h
    qbsprojectparser.cpp qbsprojectparser.h
    qbssession.cpp qbssession.h
    qbssettings.cpp qbssettings.h
)
