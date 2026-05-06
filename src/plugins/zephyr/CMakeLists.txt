add_qtc_plugin(Zephyr
  PLUGIN_DEPENDS Core ProjectExplorer CMakeProjectManager Debugger QtSupport
  SOURCES
    westbuildstep.cpp westbuildstep.h
    zephyrconstants.h
    zephyrdebug.cpp zephyrdebug.h
    zephyrplugin.cpp
    zephyrproject.cpp zephyrproject.h
    zephyrrun.cpp zephyrrun.h
    zephyrsettings.cpp zephyrsettings.h
    zephyrtr.h
)
