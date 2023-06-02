add_qtc_plugin(Haskell
  PLUGIN_DEPENDS
    QtCreator::Core QtCreator::TextEditor QtCreator::ProjectExplorer
  SOURCES
    haskell.qrc
    haskell_global.h
    haskellbuildconfiguration.cpp haskellbuildconfiguration.h
    haskellconstants.h
    haskelleditorfactory.cpp haskelleditorfactory.h
    haskellhighlighter.cpp haskellhighlighter.h
    haskellmanager.cpp haskellmanager.h
    haskellplugin.cpp haskellplugin.h
    haskellproject.cpp haskellproject.h
    haskellrunconfiguration.cpp haskellrunconfiguration.h
    haskellsettings.cpp haskellsettings.h
    haskelltokenizer.cpp haskelltokenizer.h
    haskelltr.h
    stackbuildstep.cpp stackbuildstep.h
)

qtc_add_resources(Haskell haskell_wizards
  PREFIX "/haskell"
  BASE   share/wizards
  FILES
    module/file.hs
    module/wizard.json
)
