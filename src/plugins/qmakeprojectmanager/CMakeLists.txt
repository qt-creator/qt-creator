add_qtc_plugin(QmakeProjectManager
  DEPENDS QmlJS
  PLUGIN_DEPENDS Core CppEditor QtSupport ResourceEditor TextEditor
  SOURCES
    addlibrarywizard.cpp addlibrarywizard.h
    customwidgetwizard/classdefinition.cpp customwidgetwizard/classdefinition.h
    customwidgetwizard/classlist.cpp customwidgetwizard/classlist.h
    customwidgetwizard/customwidgetpluginwizardpage.cpp customwidgetwizard/customwidgetpluginwizardpage.h
    customwidgetwizard/customwidgetwidgetswizardpage.cpp customwidgetwizard/customwidgetwidgetswizardpage.h
    customwidgetwizard/customwidgetwizard.cpp customwidgetwizard/customwidgetwizard.h
    customwidgetwizard/customwidgetwizarddialog.cpp customwidgetwizard/customwidgetwizarddialog.h
    customwidgetwizard/filenamingparameters.h
    customwidgetwizard/plugingenerator.cpp customwidgetwizard/plugingenerator.h
    customwidgetwizard/pluginoptions.h
    librarydetailscontroller.cpp librarydetailscontroller.h
    makefileparse.cpp makefileparse.h
    profilecompletionassist.cpp profilecompletionassist.h
    profileeditor.cpp profileeditor.h
    profilehighlighter.cpp profilehighlighter.h
    profilehoverhandler.cpp profilehoverhandler.h
    qmakebuildconfiguration.cpp qmakebuildconfiguration.h
    qmakebuildinfo.h
    qmakekitaspect.cpp qmakekitaspect.h
    qmakemakestep.cpp qmakemakestep.h
    qmakenodes.cpp qmakenodes.h
    qmakenodetreebuilder.cpp qmakenodetreebuilder.h
    qmakeparser.cpp qmakeparser.h
    qmakeparsernodes.cpp qmakeparsernodes.h
    qmakeproject.cpp qmakeproject.h
    qmakeprojectimporter.cpp qmakeprojectimporter.h
    qmakeprojectmanager.qrc
    qmakeprojectmanager_global.h
    qmakeprojectmanagertr.h
    qmakeprojectmanagerconstants.h
    qmakeprojectmanagerplugin.cpp qmakeprojectmanagerplugin.h
    qmakesettings.cpp qmakesettings.h
    qmakestep.cpp qmakestep.h
    wizards/qtprojectparameters.cpp wizards/qtprojectparameters.h
    wizards/qtwizard.cpp wizards/qtwizard.h
    wizards/subdirsprojectwizard.cpp wizards/subdirsprojectwizard.h
    wizards/subdirsprojectwizarddialog.cpp wizards/subdirsprojectwizarddialog.h
    wizards/wizards.qrc
)
