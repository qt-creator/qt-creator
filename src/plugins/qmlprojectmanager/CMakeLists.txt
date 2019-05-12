add_qtc_plugin(QmlProjectManager
  DEPENDS qmljs
  PLUGIN_DEPENDS Core ProjectExplorer QtSupport
  SOURCES
    fileformat/filefilteritems.cpp fileformat/filefilteritems.h
    fileformat/qmlprojectfileformat.cpp fileformat/qmlprojectfileformat.h
    fileformat/qmlprojectitem.cpp fileformat/qmlprojectitem.h
    qmlproject.cpp qmlproject.h
    qmlproject.qrc
    qmlprojectconstants.h
    qmlprojectmanager_global.h
    qmlprojectmanagerconstants.h
    qmlprojectnodes.cpp qmlprojectnodes.h
    qmlprojectplugin.cpp qmlprojectplugin.h
    qmlprojectrunconfiguration.cpp qmlprojectrunconfiguration.h
)
