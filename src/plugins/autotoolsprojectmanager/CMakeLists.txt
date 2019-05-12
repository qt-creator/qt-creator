add_qtc_plugin(AutotoolsProjectManager
  PLUGIN_DEPENDS Core CppTools ProjectExplorer QtSupport
  SOURCES
    autogenstep.cpp autogenstep.h
    autoreconfstep.cpp autoreconfstep.h
    autotoolsbuildconfiguration.cpp autotoolsbuildconfiguration.h
    autotoolsopenprojectwizard.cpp autotoolsopenprojectwizard.h
    autotoolsproject.cpp autotoolsproject.h
    autotoolsprojectconstants.h
    autotoolsprojectplugin.cpp autotoolsprojectplugin.h
    configurestep.cpp configurestep.h
    makefileparser.cpp makefileparser.h
    makefileparserthread.cpp makefileparserthread.h
    makestep.cpp makestep.h
)
