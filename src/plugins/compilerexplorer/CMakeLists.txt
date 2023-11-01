add_qtc_plugin(CompilerExplorer
  PLUGIN_DEPENDS Core ProjectExplorer TextEditor
  DEPENDS Qt::Network TerminalLib Spinner
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
    api/request.h

    compilerexplorer.qrc

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

    logos/logos.qrc
)
