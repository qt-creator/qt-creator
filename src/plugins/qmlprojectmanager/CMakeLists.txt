add_qtc_plugin(QmlProjectManager
  CONDITION TARGET Qt5::QuickWidgets
  PLUGIN_CLASS QmlProjectPlugin
  DEPENDS QmlJS Qt5::QuickWidgets
  PLUGIN_DEPENDS Core ProjectExplorer QtSupport
  SOURCES
    fileformat/filefilteritems.cpp fileformat/filefilteritems.h
    fileformat/qmlprojectfileformat.cpp fileformat/qmlprojectfileformat.h
    fileformat/qmlprojectitem.cpp fileformat/qmlprojectitem.h
    cmakegen/checkablefiletreeitem.cpp cmakegen/checkablefiletreeitem.h
    cmakegen/cmakegeneratordialog.cpp cmakegen/cmakegeneratordialog.h
    cmakegen/cmakegeneratordialogtreemodel.cpp cmakegen/cmakegeneratordialogtreemodel.h
    cmakegen/cmakeprojectconverter.cpp cmakegen/cmakeprojectconverter.h
    cmakegen/cmakeprojectconverterdialog.cpp cmakegen/cmakeprojectconverterdialog.h
    cmakegen/generatecmakelists.cpp cmakegen/generatecmakelists.h
    cmakegen/generatecmakelistsconstants.h
    cmakegen/boilerplate.qrc
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
