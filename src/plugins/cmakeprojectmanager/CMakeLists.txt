add_qtc_plugin(CMakeProjectManager
  DEPENDS qmljs
  PLUGIN_DEPENDS Core CppTools ProjectExplorer TextEditor QtSupport
  PLUGIN_RECOMMENDS Designer
  SOURCES
    builddirmanager.cpp builddirmanager.h
    builddirparameters.cpp builddirparameters.h
    builddirreader.cpp builddirreader.h
    cmake_global.h
    cmakeautocompleter.cpp cmakeautocompleter.h
    cmakebuildconfiguration.cpp cmakebuildconfiguration.h
    cmakebuildsettingswidget.cpp cmakebuildsettingswidget.h
    cmakebuildstep.cpp cmakebuildstep.h
    cmakebuildtarget.h
    cmakecbpparser.cpp cmakecbpparser.h
    cmakeconfigitem.cpp cmakeconfigitem.h
    cmakeeditor.cpp cmakeeditor.h
    cmakefilecompletionassist.cpp cmakefilecompletionassist.h
    cmakeindenter.cpp cmakeindenter.h
    cmakekitinformation.cpp cmakekitinformation.h
    cmakelocatorfilter.cpp cmakelocatorfilter.h
    cmakeparser.cpp cmakeparser.h
    cmakeprocess.cpp cmakeprocess.h
    cmakeproject.cpp cmakeproject.h
    cmakeproject.qrc
    cmakeprojectconstants.h
    cmakeprojectimporter.cpp cmakeprojectimporter.h
    cmakeprojectmanager.cpp cmakeprojectmanager.h
    cmakeprojectnodes.cpp cmakeprojectnodes.h
    cmakeprojectplugin.cpp cmakeprojectplugin.h
    cmakerunconfiguration.cpp cmakerunconfiguration.h
    cmakesettingspage.cpp cmakesettingspage.h
    cmakespecificsettings.cpp cmakespecificsettings.h
    cmakespecificsettingspage.cpp cmakespecificsettingspage.h cmakespecificsettingspage.ui
    cmaketool.cpp cmaketool.h
    cmaketoolmanager.cpp cmaketoolmanager.h
    cmaketoolsettingsaccessor.cpp cmaketoolsettingsaccessor.h
    configmodel.cpp configmodel.h
    configmodelitemdelegate.cpp configmodelitemdelegate.h
    projecttreehelper.cpp projecttreehelper.h
    servermode.cpp servermode.h
    servermodereader.cpp servermodereader.h
    tealeafreader.cpp tealeafreader.h
)
