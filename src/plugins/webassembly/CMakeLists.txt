add_qtc_plugin(WebAssembly
  DEPENDS Qt::Core
  PLUGIN_DEPENDS Core ProjectExplorer QtSupport
  SOURCES
    webassembly_global.h
    webassemblyconstants.h
    webassemblydevice.cpp webassemblydevice.h
    webassemblyemsdk.cpp webassemblyemsdk.h
    webassemblyplugin.cpp
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

qtc_add_resources(WebAssembly "images"
  PREFIX "/webassembly"
  BASE "."
  FILES
    images/webassemblydevice.png
    images/webassemblydevice@2x.png
    images/webassemblydevicesmall.png
    images/webassemblydevicesmall@2x.png
)
