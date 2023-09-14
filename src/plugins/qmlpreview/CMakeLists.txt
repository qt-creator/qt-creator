add_qtc_plugin(QmlPreview
  CONDITION TARGET QmlProjectManager
  PUBLIC_DEPENDS QmlDebug
  DEPENDS QmlJS Qt::QmlPrivate
  PLUGIN_DEPENDS
    Core ProjectExplorer QmlJSTools QtSupport
    ResourceEditor QmlProjectManager
  SOURCES
    qmlpreviewclient.cpp qmlpreviewclient.h
    qmlpreviewconnectionmanager.cpp qmlpreviewconnectionmanager.h
    qmlpreviewfileontargetfinder.cpp qmlpreviewfileontargetfinder.h
    qmlpreviewplugin.cpp qmlpreviewplugin.h
    qmlpreviewruncontrol.cpp qmlpreviewruncontrol.h
    qmldebugtranslationclient.cpp qmldebugtranslationclient.h
    qmlpreview_global.h
    qmlpreviewtr.h
)

extend_qtc_plugin(QmlPreview
  CONDITION WITH_TESTS
  SOURCES
    tests/qmlpreviewclient_test.cpp tests/qmlpreviewclient_test.h
    tests/qmlpreviewplugin_test.cpp tests/qmlpreviewplugin_test.h
)
