add_qtc_plugin(ClangPchManager
  CONDITION TARGET libclang
  DEPENDS ClangSupport CPlusPlus
  DEFINES CLANGPCHMANAGER_LIB
  PLUGIN_DEPENDS Core CppTools
  SOURCES
    clangpchmanager_global.h
    clangpchmanagerplugin.cpp clangpchmanagerplugin.h
    pchmanagerclient.cpp pchmanagerclient.h
    pchmanagerconnectionclient.cpp pchmanagerconnectionclient.h
    pchmanagernotifierinterface.cpp pchmanagernotifierinterface.h
    pchmanagerprojectupdater.cpp pchmanagerprojectupdater.h
    progressmanager.h
    progressmanagerinterface.h
    projectupdater.cpp projectupdater.h
    qtcreatorprojectupdater.cpp qtcreatorprojectupdater.h
)
