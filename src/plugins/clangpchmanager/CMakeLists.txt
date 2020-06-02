add_qtc_plugin(ClangPchManager
  BUILD_DEFAULT OFF
  CONDITION TARGET libclang
  DEPENDS ClangSupport CPlusPlus
  DEFINES CLANGPCHMANAGER_LIB
  PLUGIN_DEPENDS Core CppTools
  SOURCES
    clangindexingprojectsettings.cpp clangindexingprojectsettings.h
    clangindexingprojectsettingswidget.cpp clangindexingprojectsettingswidget.h clangindexingprojectsettingswidget.ui
    clangindexingsettingsmanager.cpp clangindexingsettingsmanager.h
    clangpchmanager_global.h
    clangpchmanagerplugin.cpp clangpchmanagerplugin.h
    pchmanagerclient.cpp pchmanagerclient.h
    pchmanagerconnectionclient.cpp pchmanagerconnectionclient.h
    pchmanagernotifierinterface.cpp pchmanagernotifierinterface.h
    pchmanagerprojectupdater.cpp pchmanagerprojectupdater.h
    preprocessormacrocollector.cpp preprocessormacrocollector.h
    preprocessormacrowidget.cpp preprocessormacrowidget.h
    progressmanager.h
    progressmanagerinterface.h
    projectupdater.cpp projectupdater.h
    qtcreatorprojectupdater.cpp qtcreatorprojectupdater.h
)
