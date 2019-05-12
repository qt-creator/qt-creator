add_qtc_plugin(QmlPreview
  DEPENDS QmlDebug qmljs
  PLUGIN_DEPENDS Core ProjectExplorer QmlJSTools QtSupport ResourceEditor
  SOURCES
    qmlpreviewclient.cpp qmlpreviewclient.h
    qmlpreviewconnectionmanager.cpp qmlpreviewconnectionmanager.h
    qmlpreviewfileontargetfinder.cpp qmlpreviewfileontargetfinder.h
    qmlpreviewplugin.cpp qmlpreviewplugin.h
    qmlpreviewruncontrol.cpp qmlpreviewruncontrol.h
    qmlpreview_global.h
)

if (WITH_TESTS)
  target_sources(QmlPreview PRIVATE
    tests/qmlpreviewclient_test.cpp tests/qmlpreviewclient_test.h
    tests/qmlpreviewplugin_test.cpp tests/qmlpreviewplugin_test.h
  )
endif()
