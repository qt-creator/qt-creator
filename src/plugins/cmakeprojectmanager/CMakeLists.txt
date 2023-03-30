add_qtc_plugin(CMakeProjectManager
  PLUGIN_CLASS CMakeProjectPlugin
  DEPENDS QmlJS
  PLUGIN_DEPENDS Core CppEditor ProjectExplorer TextEditor QtSupport
  SOURCES
    builddirparameters.cpp builddirparameters.h
    cmake_global.h
    cmakeabstractprocessstep.cpp cmakeabstractprocessstep.h
    cmakeautocompleter.cpp cmakeautocompleter.h
    cmakebuildconfiguration.cpp cmakebuildconfiguration.h
    cmakebuildsystem.cpp cmakebuildsystem.h
    cmakebuildstep.cpp cmakebuildstep.h
    cmakebuildtarget.h
    cmakeconfigitem.cpp cmakeconfigitem.h
    cmakeeditor.cpp cmakeeditor.h
    cmakefilecompletionassist.cpp cmakefilecompletionassist.h
    cmakeformatter.cpp cmakeformatter.h
    cmakeformatteroptionspage.cpp cmakeformatteroptionspage.h
    cmakeformattersettings.cpp cmakeformattersettings.h
    cmakeindenter.cpp cmakeindenter.h
    cmakeinstallstep.cpp cmakeinstallstep.h
    cmakekitinformation.cpp cmakekitinformation.h
    cmakelocatorfilter.cpp cmakelocatorfilter.h
    cmakeparser.cpp cmakeparser.h
    cmakeprocess.cpp cmakeprocess.h
    cmakeproject.cpp cmakeproject.h
    cmakeproject.qrc
    cmakeprojectconstants.h
    cmakeprojectimporter.cpp cmakeprojectimporter.h
    cmakeprojectmanager.cpp cmakeprojectmanager.h
    cmakeprojectmanagertr.h
    cmakeprojectnodes.cpp cmakeprojectnodes.h
    cmakeprojectplugin.cpp cmakeprojectplugin.h
    cmakesettingspage.cpp cmakesettingspage.h
    cmakespecificsettings.cpp cmakespecificsettings.h
    cmaketool.cpp cmaketool.h
    cmaketoolmanager.cpp cmaketoolmanager.h
    cmaketoolsettingsaccessor.cpp cmaketoolsettingsaccessor.h
    configmodel.cpp configmodel.h
    configmodelitemdelegate.cpp configmodelitemdelegate.h
    fileapidataextractor.cpp fileapidataextractor.h
    fileapiparser.cpp fileapiparser.h
    fileapireader.cpp fileapireader.h
    presetsparser.cpp presetsparser.h
    presetsmacros.cpp presetsmacros.h
    projecttreehelper.cpp projecttreehelper.h
)
