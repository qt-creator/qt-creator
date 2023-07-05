add_qtc_plugin(AutotoolsProjectManager
  PLUGIN_CLASS AutotoolsProjectPlugin
  PLUGIN_DEPENDS Core CppEditor ProjectExplorer QtSupport
  SOURCES
    autogenstep.cpp autogenstep.h
    autoreconfstep.cpp autoreconfstep.h
    autotoolsbuildconfiguration.cpp autotoolsbuildconfiguration.h
    autotoolsbuildsystem.cpp autotoolsbuildsystem.h
    autotoolsprojectconstants.h
    autotoolsprojectmanagertr.h
    autotoolsprojectplugin.cpp
    configurestep.cpp configurestep.h
    makefileparser.cpp makefileparser.h
    makefileparserthread.cpp makefileparserthread.h
    makestep.cpp makestep.h
)
