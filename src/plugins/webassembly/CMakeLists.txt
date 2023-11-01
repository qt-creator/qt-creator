add_qtc_plugin(WebAssembly
  DEPENDS Qt::Core
  PLUGIN_DEPENDS Core ProjectExplorer QtSupport
  SOURCES
    webassembly.qrc
    webassembly_global.h
    webassemblyconstants.h
    webassemblydevice.cpp webassemblydevice.h
    webassemblyemsdk.cpp webassemblyemsdk.h
    webassemblyplugin.cpp webassemblyplugin.h
    webassemblyqtversion.cpp webassemblyqtversion.h
    webassemblyrunconfiguration.cpp webassemblyrunconfiguration.h
    webassemblysettings.cpp webassemblysettings.h
    webassemblytoolchain.cpp webassemblytoolchain.h
    webassemblytr.h
)

extend_qtc_plugin(WebAssembly
  CONDITION WITH_TESTS
  SOURCES
    webassembly_test.cpp
    webassembly_test.h
)
