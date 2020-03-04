add_qtc_plugin(QmakeProjectManager
  DEPENDS QmlJS
  PLUGIN_DEPENDS Core CppTools QtSupport ResourceEditor TextEditor
  PLUGIN_RECOMMENDS Designer
  SOURCES
    addlibrarywizard.cpp addlibrarywizard.h
    applicationlauncher.h
    customwidgetwizard/classdefinition.cpp customwidgetwizard/classdefinition.h customwidgetwizard/classdefinition.ui
    customwidgetwizard/classlist.cpp customwidgetwizard/classlist.h
    customwidgetwizard/customwidgetpluginwizardpage.cpp customwidgetwizard/customwidgetpluginwizardpage.h customwidgetwizard/customwidgetpluginwizardpage.ui
    customwidgetwizard/customwidgetwidgetswizardpage.cpp customwidgetwizard/customwidgetwidgetswizardpage.h customwidgetwizard/customwidgetwidgetswizardpage.ui
    customwidgetwizard/customwidgetwizard.cpp customwidgetwizard/customwidgetwizard.h
    customwidgetwizard/customwidgetwizarddialog.cpp customwidgetwizard/customwidgetwizarddialog.h
    customwidgetwizard/filenamingparameters.h
    customwidgetwizard/plugingenerator.cpp customwidgetwizard/plugingenerator.h
    customwidgetwizard/pluginoptions.h
    externaleditors.cpp externaleditors.h
    librarydetailscontroller.cpp librarydetailscontroller.h
    librarydetailswidget.ui
    makefileparse.cpp makefileparse.h
    profilecompletionassist.cpp profilecompletionassist.h
    profileeditor.cpp profileeditor.h
    profilehighlighter.cpp profilehighlighter.h
    profilehoverhandler.cpp profilehoverhandler.h
    qmakebuildconfiguration.cpp qmakebuildconfiguration.h
    qmakebuildinfo.h
    qmakekitinformation.cpp qmakekitinformation.h
    qmakemakestep.cpp qmakemakestep.h
    qmakenodes.cpp qmakenodes.h
    qmakenodetreebuilder.cpp qmakenodetreebuilder.h
    qmakeparser.cpp qmakeparser.h
    qmakeparsernodes.cpp qmakeparsernodes.h
    qmakeproject.cpp qmakeproject.h
    qmakeprojectimporter.cpp qmakeprojectimporter.h
    qmakeprojectmanager.qrc
    qmakeprojectmanager_global.h
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
