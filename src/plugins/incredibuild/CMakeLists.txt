add_qtc_plugin(IncrediBuild
  PLUGIN_DEPENDS Core ProjectExplorer
  PLUGIN_RECOMMENDS QmakeProjectManager CMakeProjectManager
  SOURCES
  buildconsolebuildstep.cpp
  buildconsolebuildstep.h
  cmakecommandbuilder.cpp
  cmakecommandbuilder.h
  commandbuilder.cpp
  commandbuilder.h
  commandbuilderaspect.cpp
  commandbuilderaspect.h
  ibconsolebuildstep.cpp
  ibconsolebuildstep.h
  incredibuild_global.h
  incredibuildconstants.h
  incredibuildplugin.cpp
  incredibuildtr.h
  makecommandbuilder.cpp
  makecommandbuilder.h
)

