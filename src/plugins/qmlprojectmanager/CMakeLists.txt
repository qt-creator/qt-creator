add_qtc_plugin(QmlProjectManager
  PLUGIN_CLASS QmlProjectPlugin
  DEPENDS QmlJS Qt5::QuickWidgets
  PLUGIN_DEPENDS Core ProjectExplorer QtSupport
  SOURCES
    fileformat/filefilteritems.cpp fileformat/filefilteritems.h
    fileformat/qmlprojectfileformat.cpp fileformat/qmlprojectfileformat.h
    fileformat/qmlprojectitem.cpp fileformat/qmlprojectitem.h
    projectfilecontenttools.cpp projectfilecontenttools.h
    qdslandingpage.cpp qdslandingpage.h
    qmlmainfileaspect.cpp qmlmainfileaspect.h
    qmlmultilanguageaspect.cpp qmlmultilanguageaspect.h
    qmlproject.cpp qmlproject.h
    qmlproject.qrc
    qmlprojectconstants.h
    qmlprojectmanager_global.h
    qmlprojectmanagerconstants.h
    qmlprojectnodes.cpp qmlprojectnodes.h
    qmlprojectplugin.cpp qmlprojectplugin.h
    qmlprojectrunconfiguration.cpp qmlprojectrunconfiguration.h
)
