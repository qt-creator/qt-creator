add_qtc_plugin(QmlProjectManager
  CONDITION TARGET Qt::QuickWidgets
  PLUGIN_CLASS QmlProjectPlugin
  DEPENDS QmlJS Qt::QuickWidgets Utils
  PLUGIN_DEPENDS Core ProjectExplorer QtSupport QmlDesignerBase
  SOURCES
    qmlprojectgen/qmlprojectgenerator.cpp qmlprojectgen/qmlprojectgenerator.h
    qmlprojectgen/templates.qrc
    projectfilecontenttools.cpp projectfilecontenttools.h
    qdslandingpage.cpp qdslandingpage.h
    qdslandingpagetheme.cpp qdslandingpagetheme.h
    qmlmainfileaspect.cpp qmlmainfileaspect.h
    qmlmultilanguageaspect.cpp qmlmultilanguageaspect.h
    qmlproject.cpp qmlproject.h
    qmlproject.qrc
    qmlprojectconstants.h
    qmlprojectmanager_global.h
    qmlprojectmanagertr.h
    qmlprojectmanagerconstants.h
    qmlprojectplugin.cpp qmlprojectplugin.h
    qmlprojectrunconfiguration.cpp qmlprojectrunconfiguration.h
    buildsystem/qmlbuildsystem.cpp buildsystem/qmlbuildsystem.h

    "${PROJECT_SOURCE_DIR}/src/share/3rdparty/studiofonts/studiofonts.qrc"
)

extend_qtc_plugin(QmlProjectManager
    CONDITION ENABLE_COMPILE_WARNING_AS_ERROR
    PROPERTIES COMPILE_WARNING_AS_ERROR ON
)

extend_qtc_plugin(QmlProjectManager
  PUBLIC_INCLUDES ${CMAKE_CURRENT_LIST_DIR}/buildsystem
  SOURCES_PREFIX ${CMAKE_CURRENT_LIST_DIR}/buildsystem
  SOURCES
      projectitem/filefilteritems.cpp projectitem/filefilteritems.h
      projectitem/qmlprojectitem.cpp projectitem/qmlprojectitem.h
      projectitem/converters.h projectitem/converters.cpp
      projectnode/qmlprojectnodes.cpp projectnode/qmlprojectnodes.h
)

extend_qtc_plugin(QmlProjectManager
  PUBLIC_INCLUDES ${CMAKE_CURRENT_LIST_DIR}/cmakegen
  SOURCES_PREFIX ${CMAKE_CURRENT_LIST_DIR}/cmakegen
  SOURCES
      checkablefiletreeitem.cpp checkablefiletreeitem.h
      cmakegeneratordialog.cpp cmakegeneratordialog.h
      cmakegeneratordialogtreemodel.cpp cmakegeneratordialogtreemodel.h
      cmakeprojectconverter.cpp cmakeprojectconverter.h
      cmakeprojectconverterdialog.cpp cmakeprojectconverterdialog.h
      generatecmakelists.cpp generatecmakelists.h
      generatecmakelistsconstants.h
      boilerplate.qrc
)

add_qtc_library(QmlProjectManagerLib OBJECT
    CONDITION Qt6_VERSION VERSION_GREATER_EQUAL 6.4.3
    EXCLUDE_FROM_INSTALL
    DEPENDS
        QmlJS Utils
    INCLUDES
        ${CMAKE_CURRENT_LIST_DIR}
    SOURCES_PREFIX ${CMAKE_CURRENT_LIST_DIR}/buildsystem
    SOURCES
        projectitem/filefilteritems.cpp projectitem/filefilteritems.h
        projectitem/qmlprojectitem.cpp projectitem/qmlprojectitem.h
        projectitem/converters.cpp projectitem/converters.h
)
