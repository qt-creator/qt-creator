add_qtc_plugin(AutotoolsProjectManager
  PLUGIN_CLASS AutotoolsProjectPlugin
  PLUGIN_DEPENDS Core ProjectExplorer QtSupport
  SOURCES
    autogenstep.cpp autogenstep.h
    autoreconfstep.cpp autoreconfstep.h
    autotoolsbuildconfiguration.cpp autotoolsbuildconfiguration.h
    autotoolsprojectconstants.h
    autotoolsprojectmanagertr.h
    autotoolsprojectplugin.cpp
    configurestep.cpp configurestep.h
    makefileparser.cpp makefileparser.h
    makestep.cpp makestep.h
)
