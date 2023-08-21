add_qtc_plugin(QbsProjectManager
  DEPENDS Qt::Qml Qt::Widgets QmlJS
  DEFINES
    IDE_LIBRARY_BASENAME="${IDE_LIBRARY_BASE_PATH}"
  PLUGIN_DEPENDS Core ProjectExplorer CppEditor QtSupport QmlJSTools
  SOURCES
    customqbspropertiesdialog.cpp customqbspropertiesdialog.h
    defaultpropertyprovider.cpp defaultpropertyprovider.h
    propertyprovider.h
    qbsbuildconfiguration.cpp qbsbuildconfiguration.h
    qbsbuildstep.cpp qbsbuildstep.h
    qbscleanstep.cpp qbscleanstep.h
    qbsinstallstep.cpp qbsinstallstep.h
    qbskitaspect.cpp qbskitaspect.h
    qbsnodes.cpp qbsnodes.h
    qbsnodetreebuilder.cpp qbsnodetreebuilder.h
    qbspmlogging.cpp qbspmlogging.h
    qbsprofilemanager.cpp qbsprofilemanager.h
    qbsprofilessettingspage.cpp qbsprofilessettingspage.h
    qbsproject.cpp qbsproject.h
    qbsprojectimporter.cpp qbsprojectimporter.h
    qbsprojectmanager.qrc
    qbsprojectmanager_global.h
    qbsprojectmanagertr.h
    qbsprojectmanagerconstants.h
    qbsprojectmanagerplugin.cpp qbsprojectmanagerplugin.h
    qbsprojectparser.cpp qbsprojectparser.h
    qbsrequest.cpp qbsrequest.h
    qbssession.cpp qbssession.h
    qbssettings.cpp qbssettings.h
)
