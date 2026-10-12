add_qtc_plugin(CompilerExplorer
  PLUGIN_DEPENDS Core ProjectExplorer TextEditor
  DEPENDS Qt::Network TerminalLib
  SOURCES
    api/config.h
    api/compile.cpp
    api/compile.h
    api/compiler.cpp
    api/compiler.h
    api/compilerexplorerapi.h
    api/language.cpp
    api/language.h
    api/library.cpp
    api/library.h
    api/request.cpp
    api/request.h
    compilerexploreraspects.cpp
    compilerexploreraspects.h
    compilerexplorerplugin.cpp
    compilerexplorertr.h
    compilerexplorereditor.cpp
    compilerexplorereditor.h
    compilerexplorersettings.cpp
    compilerexplorersettings.h
    compilerexploreroptions.cpp
    compilerexploreroptions.h
)

qtc_add_resources(CompilerExplorer "wizards"
    PREFIX "/compilerexplorer/wizard"
    FILES
       wizard/cpp/wizard.json
       wizard/cpp/file.qtce
       wizard/python/wizard.json
       wizard/python/file.qtce
       wizard/qtcpp/file.qtce
       wizard/qtcpp/wizard.json
)

qtc_add_resources(CompilerExplorer "logos"
  PREFIX "/compilerexplorer/logos"
  BASE "logos"
  FILES logos/ce.ico
)
