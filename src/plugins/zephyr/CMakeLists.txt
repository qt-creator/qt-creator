add_qtc_plugin(Zephyr
  PLUGIN_DEPENDS Core ProjectExplorer Debugger QtSupport
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

qtc_add_resources(Zephyr "wizards"
  PREFIX "/zephyr"
  FILES
    wizards/application/CMakeLists.txt
    wizards/application/main.c
    wizards/application/prj.conf
    wizards/application/west.yml
    wizards/application/wizard.json
)
